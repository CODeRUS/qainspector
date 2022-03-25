// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#include "iteminfodialog.h"

#include <QJsonValue>
#include <QLayout>
#include <QLineEdit>
#include <QSettings>

ItemInfoDialog::ItemInfoDialog(QWidget* parent)
    : QDialog(parent)
{
    auto settings = new QSettings(QStringLiteral("qainspector.ini"), QSettings::IniFormat, this);
    restoreGeometry(settings->value("properties/geometry").toByteArray());
}

void ItemInfoDialog::setData(const QJsonObject& object)
{
    auto formLayout = new QVBoxLayout();
    for (auto it = object.constBegin(); it != object.constEnd(); it++)
    {
        auto lineLayout = new QHBoxLayout();

        auto keyEdit = new QLineEdit();
        keyEdit->setText(it.key());
        keyEdit->setReadOnly(true);
        keyEdit->setCursorPosition(0);
        keyEdit->installEventFilter(this);
        lineLayout->addWidget(keyEdit, 3);

        auto valueEdit = new QLineEdit();
        valueEdit->setText(it.value().toVariant().toString());
        valueEdit->setReadOnly(true);
        valueEdit->setCursorPosition(0);
        valueEdit->installEventFilter(this);
        lineLayout->addWidget(valueEdit, 2);

        formLayout->addLayout(lineLayout);
    }

    QWidget* viewport = new QWidget();
    viewport->setLayout(formLayout);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(viewport);
    scrollArea->setWidgetResizable(true);

    QHBoxLayout* hlayout = new QHBoxLayout(this);
    hlayout->addWidget(scrollArea);
}

bool ItemInfoDialog::eventFilter(QObject* o, QEvent* e)
{
    QLineEdit* l = qobject_cast<QLineEdit*>(o);
    if (!l)
    {
        return QObject::eventFilter(o, e);
    }
    if (e->type() == QEvent::MouseButtonPress)
    {
        l->selectAll();
        return true;
    }
    return QObject::eventFilter(o, e);
}

void ItemInfoDialog::closeEvent(QCloseEvent* event)
{
    auto settings = new QSettings(QStringLiteral("qainspector.ini"), QSettings::IniFormat, this);
    settings->setValue("properties/geometry", saveGeometry());
    QDialog::closeEvent(event);
}
