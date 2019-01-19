#include <iostream>

#include <QHeaderView>
#include <QMainWindow>
#include <QDebug>
#include <QModelIndexList>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QLabel>
#include <QSpinBox>
#include <QApplication>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_text.h>

#include "Charts/BasicDataPlot.h"
#include "Charts/QuantilesPlot.h"
#include "TableModel.h"
#include "FilteringProxyModel.h"
#include "DataProvider/PlotDataProvider.h"
#include "Common/SpecialColumns.h"
#include "Shared/Logger.h"
#include "Common/Constants.h"
#include "DataView.h"
#include "NumericDelegate.h"
#include "DateDelegate.h"
#include "GUI/PlotDockWidget.h"
#include "GUI/GroupPlotGui.h"

DataView::DataView(QWidget *parent) :
    QTableView(parent), plotDataProvider_(NULL)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    const int rowHeight = fontMetrics().height()*1.5;
    verticalHeader()->setDefaultSectionSize(rowHeight);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->setVisible(false);
    setSortingEnabled(true);
    horizontalHeader()->setSortIndicatorShown(false);

    horizontalHeader()->setSectionsMovable(true);

    connect(horizontalHeader(),
            SIGNAL(sectionClicked(int)),
            this,
            SLOT(showSortIndicatorIfNeeded(int)));
}

DataView::~DataView()
{
    if( NULL != plotDataProvider_ )
    {
        delete plotDataProvider_;
    }
}

void DataView::setModel(QAbstractItemModel* model)
{
    const TableModel* parentModel =
        (dynamic_cast<FilteringProxyModel*>(model))->getParentModel();
    FilteringProxyModel* proxyModel = dynamic_cast<FilteringProxyModel*>(model);

    for(int i = 0; i < proxyModel->columnCount() ; ++i)
    {
        switch(parentModel->getColumnFormat(i))
        {
            case DATA_FORMAT_FLOAT:
            {
                setItemDelegateForColumn(i, new NumericDelegate(this));
                break;
            }

            case DATA_FORMAT_DATE:
            {
                setItemDelegateForColumn(i, new DateDelegate(this));
                break;
            }

            default:
            {
                break;
            }
        }
    }

    QTableView::setModel(model);

    //Performance problem.
    //resizeColumnsToContents();
}

void DataView::groupingColumnChanged(int column)
{
    QVector<TransactionData>* newCalcData = fillDataFromSelection(column);

    const TableModel* parentModel =
        (dynamic_cast<FilteringProxyModel*>(model()))->getParentModel();

    plotDataProvider_->recomputeGroupData(newCalcData,
                                          column,
                                          parentModel->getColumnFormat(column));
}

QVector<TransactionData>* DataView::fillDataFromSelection(int groupByColumn)
{
    FilteringProxyModel* proxyModel = static_cast<FilteringProxyModel*>(model());
    Q_ASSERT(NULL != proxyModel);

    const TableModel* parentModel = proxyModel->getParentModel();
    Q_ASSERT(NULL != parentModel);

    int pricePerMeterColumn;
    if(false == parentModel->getSpecialColumnIfExists(SPECIAL_COLUMN_PRICE_PER_UNIT,
                                                      pricePerMeterColumn))
    {
        return NULL;
    }

    int transactionDateColumn;
    if(false == parentModel->getSpecialColumnIfExists(SPECIAL_COLUMN_TRANSACTION_DATE,
                                                      transactionDateColumn))
    {
        return NULL;
    }

    QTime performanceTimer;
    performanceTimer.start();

    QItemSelectionModel* selectionModelOfView = selectionModel();

    QVector<TransactionData>* calcDataContainer = new QVector<TransactionData>();

    int proxyRowCount = proxyModel->rowCount();

    for(int i = 0; i < proxyRowCount; ++i)
    {
        if ( 0 == i % 1000 )
        {
            QApplication::processEvents();
        }

        if(false == selectionModelOfView->isSelected(proxyModel->index(i, 0)))
            continue;

        TransactionData temp;
        const QVariant& data = proxyModel->index(i, transactionDateColumn).data();

        //Do not take into calculations and plots rows with empty date or price.
        if ( false == data.isNull() )
        {
            temp.pricePerMeter_ =
                proxyModel->index(i, pricePerMeterColumn).data().toFloat();
            temp.date_ = data.toDate();

            //String values can be stored as indexes of table
            //with data when it will be done.

            //Temp, remove when all types of column managed in grouping.
            if( -1 != groupByColumn )
                temp.groupedBy_ = proxyModel->index(i, groupByColumn).data();

            calcDataContainer->append(temp);
        }
    }

    LOG(LOG_CALC, "Data updated in time " +
                  QString::number(performanceTimer.elapsed()*1.0/1000) + " seconds.");

    return calcDataContainer;
}

void DataView::reloadSelectionDataAndRecompute()
{
    if( NULL == plotDataProvider_ )
    {
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    //TODO optimize by impact + depact or additionall columns in model.
    int groupByColumn = plotDataProvider_->getGroupByColumn();
    QVector<TransactionData>* newCalcData =
            fillDataFromSelection(groupByColumn);

    //Temp, until all column types managed.
    DataFormat columnFormat = DATA_FORMAT_UNKNOWN;
    if( -1 != groupByColumn)
    {
        const TableModel* parentModel =
            (dynamic_cast<FilteringProxyModel*>(model()))->getParentModel();

        columnFormat = parentModel->getColumnFormat(groupByColumn);
    }

    Q_ASSERT(NULL != newCalcData);

    QTime performanceTimer;
    performanceTimer.start();

    plotDataProvider_->reCompute(newCalcData,
                                 columnFormat);

    LOG(LOG_CALC, "Plots recomputed in " +
                  QString::number(performanceTimer.elapsed()*1.0/1000) + " seconds.");

    QApplication::restoreOverrideCursor();
}

void DataView::showSortIndicatorIfNeeded(int section)
{
    if ( false == horizontalHeader()->isSortIndicatorShown() )
    {
        horizontalHeader()->setSortIndicatorShown(true);
        model()->sort(section);
    }
}

void DataView::mouseReleaseEvent(QMouseEvent* event)
{
    QTableView::mouseReleaseEvent(event);

    if ( Qt::LeftButton == event->button() )
    {
        reloadSelectionDataAndRecompute();
    }
}

void DataView::keyPressEvent(QKeyEvent* event)
{
    QTableView::keyPressEvent(event);

    if( Qt::Key_A == event->key() && Qt::CTRL == event->modifiers() )
    {
        reloadSelectionDataAndRecompute();
    }
}

const PlotDataProvider* DataView::getPlotDataProvider()
{
    if( NULL == plotDataProvider_)
    {
        const TableModel* tableModel =
            (dynamic_cast<FilteringProxyModel*>(model()))->getParentModel();

        plotDataProvider_ =
            new PlotDataProvider(tableModel->getDefaultGroupingColumn());
    }

    return plotDataProvider_;
}
