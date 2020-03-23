#ifndef ITEMINFODIALOG_H
#define ITEMINFODIALOG_H

#include <QDialog>
#include <QEvent>
#include <QJsonObject>
#include <QScrollArea>

class ItemInfoDialog : public QDialog
{
    Q_OBJECT
public:
    ItemInfoDialog();

public slots:
    void setData(const QJsonObject &object);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
};

#endif // ITEMINFODIALOG_H
