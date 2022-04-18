// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#ifndef ITEMINFODIALOG_H
#define ITEMINFODIALOG_H

#include <QDialog>
#include <QEvent>
#include <QJsonObject>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

class ItemInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ItemInfoDialog(QWidget* parent = nullptr);

public slots:
    void setData(const QJsonObject& object);

protected:
    bool eventFilter(QObject* o, QEvent* e) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QVBoxLayout* formLayout = nullptr;
    QSettings* settings;
};

#endif // ITEMINFODIALOG_H
