#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>

#include "Shared/Logger.h"
#include "Update/UpdateProperties.h"
#include "Shared/Application.h"

#include "Update.h"

void showDifferentInstanceRunnningError()
{
    QMessageBox::critical(NULL,
                          QObject::tr("Error"),
                          QObject::tr("Different instance of ") +
                          QApplication::applicationName() +
                          QObject::tr(" already running, exiting."));

}

/**
 * @brief check if different updater is working.
 * @return true if unique, false if already working.
 */
bool isUniqueInstance()
{
    //Only one instance can run. Intentionally leak.
    QSharedMemory* sharedMemory = new QSharedMemory(QLatin1String(VER_PRODUCTNAME_STR));

    LOG(LOG_APP,
        QLatin1String("Setting shared memory key named ") +
        QApplication::applicationName() + QLatin1Char('.'));

    if(sharedMemory->attach())
    {
        LOG(LOG_APP,
            QLatin1String("Attached to shared memory. Different instance already running, exiting."));

        return false;
    }

    LOG(LOG_APP, QLatin1String("Attaching to shared memory successful. Continue."));

    if(!sharedMemory->create(1))
    {
        LOG(LOG_APP,
            QLatin1String("Creating shared memory failed. Different instance already running, exiting."));

        return false;
    }

    LOG(LOG_APP, QLatin1String("Unique instance, continue."));

    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Application::setAdditionalApplicatioInfo(VER_PRODUCTNAME_STR);
    Application::setQtStyle(QLatin1String("Fusion"));

    if( false == isUniqueInstance() )
    {
        showDifferentInstanceRunnningError();

        return -1;
    }

    Update w;
    w.show();

    return a.exec();
}
