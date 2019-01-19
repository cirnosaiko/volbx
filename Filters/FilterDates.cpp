#include <QDebug>

#include "Common/Constants.h"

#include "FilterDates.h"
#include "ui_FilterDates.h"

FilterDates::FilterDates(QString name,
                         int column,
                         QDate min,
                         QDate max,
                         bool emptyDates,
                         QWidget *parent) :
    Filter(name, column, parent),
    minOnInit_(min),
    maxOnInit_(max),
    ui(new Ui::FilterDates),
    emptyDates_(emptyDates)
{
    ui->setupUi(this);

    if(minOnInit_ == maxOnInit_)
        setDisabled(true);

    //Lower.
    calendarLeft_.setFirstDayOfWeek(Qt::Monday);
    calendarLeft_.setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    ui->lowerDateEdit->setDate(minOnInit_);
    ui->lowerDateEdit->setCalendarWidget(&calendarLeft_);
    connect(ui->lowerDateEdit,
            SIGNAL(dateChanged(QDate)),
            this,
            SLOT(lowerDateChanged(QDate)));

    //Higher.
    calendarRight_.setFirstDayOfWeek(Qt::Monday);
    calendarRight_.setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    ui->higherDateEdit->setDate(maxOnInit_);
    ui->higherDateEdit->setCalendarWidget(&calendarRight_);
    connect(ui->higherDateEdit,
            SIGNAL(dateChanged(QDate)),
            this,
            SLOT(higherDateChanged(QDate)));

    if ( false == emptyDates_ )
    {
        ui->emptyDates->hide();
    }
    else
    {
        ui->emptyDates->show();
    }
}

FilterDates::~FilterDates()
{
    delete ui;
}

void FilterDates::lowerDateChanged(QDate newDate)
{
    //signal block?
    if(newDate > ui->higherDateEdit->date())
    {
        ui->higherDateEdit->setDate(newDate);
    }

    QApplication::processEvents();

    //Emit to model via filters dock.
    emit newDateFilter(column_,
                       ui->lowerDateEdit->date(),
                       ui->higherDateEdit->date(),
                       ui->emptyDates->isChecked());
}

void FilterDates::higherDateChanged(QDate newDate)
{
    //signal block?
    if(newDate < ui->lowerDateEdit->date())
        ui->lowerDateEdit->setDate(newDate);

    QApplication::processEvents();

    //Emit to model via filters dock.
    emit newDateFilter(column_,
                       ui->lowerDateEdit->date(),
                       ui->higherDateEdit->date(),
                       ui->emptyDates->isChecked());
}

void FilterDates::setChecked(bool checked)
{
    QGroupBox::setChecked(checked);

    QList<QDateEdit*> dateWidgets = findChildren<QDateEdit*>();

    foreach(QWidget* current, dateWidgets)
    {
        current->setVisible(checked);
    }

    ui->emptyDates->setVisible(checked && emptyDates_);
}

void FilterDates::on_emptyDates_toggled(bool checked)
{
    QApplication::processEvents();
    emit newDateFilter(column_,
                       ui->lowerDateEdit->date(),
                       ui->higherDateEdit->date(),
                       checked);
}
