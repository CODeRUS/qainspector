// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#include "iteminfodialog.h"

#include <QCoreApplication>
#include <QJsonValue>
#include <QLayout>
#include <QLineEdit>
#include <QSettings>

ItemInfoDialog::ItemInfoDialog(QWidget* parent)
    : QDialog(parent)
{
    QWidget* viewport = new QWidget();
    formLayout = new QVBoxLayout();
    viewport->setLayout(formLayout);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(viewport);
    scrollArea->setWidgetResizable(true);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addWidget(scrollArea);

    setLayout(hlayout);

    auto settings = new QSettings(QStringLiteral("qainspector.ini"), QSettings::IniFormat, this);
    restoreGeometry(settings->value("properties/geometry").toByteArray());
}

void ItemInfoDialog::setData(const QJsonObject& object)
{
    while (auto* item = formLayout->takeAt(0))
        delete item;

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
