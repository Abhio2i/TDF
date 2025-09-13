#ifndef LOGGER_H
#define LOGGER_H
#include <QWidget>
#include <QStringList>

class QCheckBox;
class QPushButton;

class Logger : public QWidget
{
    Q_OBJECT
public:
    explicit Logger(QWidget *parent = nullptr);
signals:
    void startRecording();
    void stopRecording();
    void replayRecording();
    void eventTypesSelected(QStringList eventTypes);
private:
    QCheckBox *actionsCheckBox;
    QCheckBox *waypointsCheckBox;
    QCheckBox *engagementsCheckBox;
    void setupUi();
};

#endif // LOGGER_H
