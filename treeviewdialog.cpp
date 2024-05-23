// Copyright (c) 2019-2020 Open Mobile Platform LLC.
#include "treeviewdialog.h"
#include <QAbstractEventDispatcher>
#include <QAction>
#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSizePolicy>
#include <QSplitter>
#include <QThread>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

#include "iteminfodialog.h"
#include "socketconnector.h"

TreeViewDialog::TreeViewDialog()
{
    socket = new SocketConnector();
    settings = new QSettings("QA", "QAInspector", this);

    QThread *socketThread = new QThread();
    connect(qApp, &QCoreApplication::aboutToQuit, socketThread, &QThread::quit);
    connect(socketThread, &QThread::finished, socket, &QObject::deleteLater);

    socket->moveToThread(socketThread);

    connect(this, &TreeViewDialog::getDumpTree, socket, &SocketConnector::getDumpTree);
    connect(this, &TreeViewDialog::getGrabWindow, socket, &SocketConnector::getGrabWindow);
    connect(this, &TreeViewDialog::mousePressed, socket, &SocketConnector::mousePressed);
    connect(this, &TreeViewDialog::mouseReleased, socket, &SocketConnector::mouseReleased);
    connect(this, &TreeViewDialog::mouseMoved, socket, &SocketConnector::mouseMoved);

    connect(socket, &SocketConnector::dumpTreeData, this, &TreeViewDialog::dumpTreeData);
    connect(socket, &SocketConnector::dumpScreenshotData, this, &TreeViewDialog::dumpScreenshotData);

    connect(this, &TreeViewDialog::socketSwapState, socket, &SocketConnector::swapState);

    connect(socketThread, &QThread::started, socket, &SocketConnector::init);

    socketThread->start();

    treeView = new QTreeView(this);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    model = new MyTreeModel2;
    treeView->setModel(model);
    treeView->setColumnWidth(0, 300);
    treeView->setColumnWidth(1, 300);
    treeView->setColumnWidth(2, 200);
    treeView->setColumnWidth(3, 50);
    treeView->setColumnWidth(4, 50);
    treeView->setColumnWidth(5, 50);
    treeView->setColumnWidth(6, 50);
    treeView->setColumnWidth(7, 50);
    treeView->setColumnWidth(8, 50);

    treeView->header()->restoreState(settings->value("treeheader/state").toByteArray());

    connect(treeView,
            &QTreeView::customContextMenuRequested,
            this,
            &TreeViewDialog::onContextMenuRequested);
    connect(treeView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this](const QModelIndex& current, const QModelIndex&)
            { paintedWidget->setItemRect(model->getRect(current)); });

    int width = 1024;
    int height = 800;
    int screenWidth = screen()->geometry().width();
    int screenHeight = screen()->geometry().height();

    if (settings->contains("window/geometry")) {
        restoreGeometry(settings->value("window/geometry").toByteArray());
    } else {
        setGeometry((screenWidth/2)-(width/2), (screenHeight/2)-(height/2), width, height);
    }


}

QLayout* TreeViewDialog::createTopLayout()
{
    auto connectionLayout = new QHBoxLayout;
    auto ipLineEdit = new QLineEdit(this);
    connect(ipLineEdit,
            &QLineEdit::textChanged,
            this,
            [&](const QString& text)
            {
                settings->setValue(QStringLiteral("connection/host"), text);
                socket->setProperty("hostname", text);
            });
    ipLineEdit->setPlaceholderText("127.0.0.1");
    ipLineEdit->setText(
        settings->value(QStringLiteral("connection/host"), ipLineEdit->placeholderText())
            .toString());
    connectionLayout->addWidget(ipLineEdit);

    auto portLineEdit = new QLineEdit(this);
    connect(ipLineEdit,
            &QLineEdit::returnPressed,
            this,
            [portLineEdit]() { portLineEdit->setFocus(Qt::TabFocusReason); });
    connect(portLineEdit,
            &QLineEdit::textChanged,
            this,
            [&](const QString& text)
            {
                settings->setValue(QStringLiteral("connection/port"), text);
                socket->setProperty("port", text);
            });
    portLineEdit->setPlaceholderText("8888");
    portLineEdit->setText(
        settings->value(QStringLiteral("connection/port"), portLineEdit->placeholderText())
            .toString());
    connectionLayout->addWidget(portLineEdit);

    auto connectButton = new QPushButton(tr("Connect"), this);
    connectButton->setAutoDefault(true);
    connectButton->setFocusPolicy(Qt::StrongFocus);
    connect(portLineEdit,
            &QLineEdit::returnPressed,
            this,
            [connectButton]() { connectButton->setFocus(Qt::TabFocusReason); });
    connect(connectButton,
            &QPushButton::clicked,
            this,
            [=]() {
                connectButton->setEnabled(false);
                emit socketSwapState();
            });
    connectionLayout->addWidget(connectButton);

    auto dumpTreeButton = new MyPushButton(tr("Dump tree"), this);
    dumpTreeButton->setFocusPolicy(Qt::StrongFocus);
    connect(dumpTreeButton, &QPushButton::clicked, this, [=]() {
        dumpTree();
        dumpTreeButton->setEnabled(false);
    });
    connect(dumpTreeButton, &MyPushButton::shiftClicked, this, [=]() {
        QTimer::singleShot(shiftDelay, this, &TreeViewDialog::dumpTree);
    });
    connect(socket, &SocketConnector::dumpTreeData, this, [=]() {
        dumpTreeButton->setEnabled(true);
    });
    dumpTreeButton->setVisible(false);
    connectionLayout->addWidget(dumpTreeButton);

    auto expandAllButton = new QPushButton(tr("Expand all"), this);
    expandAllButton->setFocusPolicy(Qt::StrongFocus);
    connect(expandAllButton, &QPushButton::clicked, this, [=]() { treeView->expandAll(); });
    expandAllButton->setVisible(false);
    connectionLayout->addWidget(expandAllButton);

    connectionLayout->setStretch(0, 2);

    connect(socket,
            &SocketConnector::connectedChanged,
            this,
            [=](bool isSocketConnected)
            {
                connectButton->setText(isSocketConnected ? tr("Disconnect") : tr("Connect"));
                dumpTreeButton->setVisible(isSocketConnected);
                expandAllButton->setVisible(isSocketConnected);

                if (isSocketConnected)
                {
                    dumpTree();
                }

                connectButton->setEnabled(true);
            });

    connectionLayout->setAlignment(Qt::AlignLeft);

    QTimer::singleShot(
        1, this, [ipLineEdit]() { ipLineEdit->setFocus(Qt::ActiveWindowFocusReason); });

    return connectionLayout;
}

QLayout* TreeViewDialog::createSearchLayout()
{
    auto searchLayout = new QHBoxLayout;
    searchLineEdit = new QLineEdit;
    searchLineEdit->setPlaceholderText("Search pattern");
    partialCheckBox = new QCheckBox(tr("Partial"));
    partialCheckBox->setFocusPolicy(Qt::StrongFocus);

    auto groupBox = new QGroupBox;
    auto groupBoxLayout = new QHBoxLayout;
    auto classNameButton = new QRadioButton("ClassName");
    classNameButton->setFocusPolicy(Qt::StrongFocus);
    classNameButton->setChecked(true);
    connect(classNameButton,
            &QRadioButton::clicked,
            this,
            [&]() { searchType = MyTreeModel2::SearchType::ClassName; });
    auto textButton = new QRadioButton("Text");
    textButton->setFocusPolicy(Qt::StrongFocus);
    connect(textButton,
            &QRadioButton::clicked,
            this,
            [&]() { searchType = MyTreeModel2::SearchType::Text; });
    auto objectNameButton = new QRadioButton("ObjectName");
    objectNameButton->setFocusPolicy(Qt::StrongFocus);
    connect(objectNameButton,
            &QRadioButton::clicked,
            this,
            [&]() { searchType = MyTreeModel2::SearchType::ObjectName; });
    auto xPathButton = new QRadioButton("XPath");
    xPathButton->setFocusPolicy(Qt::StrongFocus);
    connect(xPathButton,
            &QRadioButton::clicked,
            this,
            [&]() { searchType = MyTreeModel2::SearchType::XPath; });
    groupBoxLayout->addWidget(classNameButton);
    groupBoxLayout->addWidget(textButton);
    groupBoxLayout->addWidget(objectNameButton);
    groupBoxLayout->addWidget(xPathButton);
    groupBox->setLayout(groupBoxLayout);

    auto searchButton = new QPushButton(tr("Search"));
    searchButton->setFocusPolicy(Qt::StrongFocus);
    connect(searchButton,
            &QPushButton::clicked,
            this,
            [&](bool)
            {
                auto result = model->searchIndex(searchType,
                                                 searchLineEdit->text(),
                                                 partialCheckBox->isChecked(),
                                                 model->index(treeView->currentIndex().row(),
                                                              0,
                                                              treeView->currentIndex().parent()));
                selectSearchResult(result);
            });

    QShortcut* searchShortcut = new QShortcut(QKeySequence("F3"), this);
    connect(searchShortcut,
            &QShortcut::activated,
            this,
            [&]()
            {
                auto result = model->searchIndex(searchType,
                                                 searchLineEdit->text(),
                                                 partialCheckBox->isChecked(),
                                                 model->index(treeView->currentIndex().row(),
                                                              0,
                                                              treeView->currentIndex().parent()));
                selectSearchResult(result);
            });

    QShortcut* openSearchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(openSearchShortcut,
            &QShortcut::activated,
            this,
            [&]() { searchLineEdit->setFocus(Qt::ShortcutFocusReason); });

    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(partialCheckBox);
    searchLayout->addWidget(groupBox);
    searchLayout->addWidget(searchButton);

    connect(model,
            &QAbstractItemModel::modelReset,
            this,
            [=]() { searchButton->setEnabled(model->rowCount() > 0); });
    searchButton->setEnabled(false);

    return searchLayout;
}

void TreeViewDialog::closeEvent(QCloseEvent* event)
{
    if (dialog && dialog->isVisible())
    {
        dialog->close();
    }

    settings->setValue("window/geometry", saveGeometry());
    settings->setValue("treeheader/state", treeView->header()->saveState());
    settings->sync();

    QWidget::closeEvent(event);
}

void TreeViewDialog::init()
{
    if (qApp->arguments().size() > 1) {
        QString arg = qApp->arguments().at(1);
        int temp = arg.toInt();
        if (temp > 0) {
            shiftDelay = temp;
            qDebug() << Q_FUNC_INFO << "Shift delay set to:" << shiftDelay;
        }
    }

    auto formLayout = new QVBoxLayout;
    formLayout->addLayout(createTopLayout());

    auto leftFrame = new QFrame();

    paintedWidget = new PaintedWidget(leftFrame);
    paintedWidget->installEventFilter(this);
    leftFrame->setMinimumWidth(100);

    treeView->setMinimumWidth(100);

    auto splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(leftFrame);
    splitter->addWidget(treeView);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setStretchFactor(1, 1);

    splitter->restoreState(settings->value("splitterSizes").toByteArray());

    connect(splitter,
            &QSplitter::splitterMoved,
            this,
            [splitter, this](int, int)
            { settings->setValue("splitterSizes", splitter->saveState()); });

    formLayout->addWidget(splitter);

    formLayout->addLayout(createSearchLayout());

    setLayout(formLayout);

    QFile file("dump.json");
    if (file.exists())
    {
        file.open(QIODevice::ReadOnly);
        model->loadDump(file.readAll());
    }
}

void expandChildren(const QModelIndex& index, QTreeView* view)
{
    if (!index.isValid())
    {
        return;
    }

    auto current = index;
    while (current.isValid())
    {
        view->setExpanded(current, true);
        current = current.parent();
    }
}

void TreeViewDialog::selectSearchResult(const QModelIndex& index)
{
    qDebug() << Q_FUNC_INFO << index;
    if (index.isValid())
    {
        treeView->selectionModel()->clear();
        expandChildren(index, treeView);
        treeView->setCurrentIndex(index);
        treeView->scrollTo(index);
        treeView->selectionModel()->select(QItemSelection(index, index),
                                           QItemSelectionModel::ClearAndSelect |
                                               QItemSelectionModel::Rows);
    }
}

void TreeViewDialog::onContextMenuRequested(const QPoint&)
{
    if (!dialog)
    {
        dialog = new ItemInfoDialog();
        dialog->setWindowTitle(tr("properties list"));
    }
    dialog->setData(model->getData(treeView->currentIndex()));
    dialog->show();
    dialog->raise();
}

bool TreeViewDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* me = dynamic_cast<QMouseEvent*>(event);
        if (!me) {
            return QObject::eventFilter(obj, event);
        }
        QPoint point = me->localPos().toPoint();

        if (me->button() == Qt::LeftButton) {
            paintedWidget->setClickPoint(point);

            QModelIndex index = model->searchByCoordinates(paintedWidget->scaledClickPoint());
            if (index.isValid())
            {
                selectSearchResult(index);
            }
        } else {
            QPoint scaledPoint(point.x() / paintedWidget->scaleRatio(), point.y() / paintedWidget->scaleRatio());
            emit mousePressed(scaledPoint);
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* me = dynamic_cast<QMouseEvent*>(event);
        if (!me) {
            return QObject::eventFilter(obj, event);
        }
        QPoint point = me->localPos().toPoint();

        if (me->button() == Qt::RightButton) {
            QPoint scaledPoint(point.x() / paintedWidget->scaleRatio(), point.y() / paintedWidget->scaleRatio());
            emit mouseReleased(scaledPoint);
            QTimer::singleShot(300, this, &TreeViewDialog::dumpScreenshot);
        }

    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent* me = dynamic_cast<QMouseEvent*>(event);
        if (!me) {
            return QObject::eventFilter(obj, event);
        }
        QPoint point = me->localPos().toPoint();

        if (me->buttons() & Qt::RightButton) {
            QPoint scaledPoint(point.x() / paintedWidget->scaleRatio(), point.y() / paintedWidget->scaleRatio());
            emit mouseMoved(scaledPoint);
        }
    }

    return QObject::eventFilter(obj, event);
}

void TreeViewDialog::dumpTree()
{
    emit getDumpTree();
}

void TreeViewDialog::dumpScreenshot()
{
    emit getGrabWindow();

//    if (socket->isConnected()) {
//        QTimer::singleShot(100, this, &TreeViewDialog::dumpScreenshot);
//    }
}

void TreeViewDialog::dumpTreeData(const QByteArray &data)
{
    if (!data.isEmpty())
    {
        model->loadDump(data);
    }
    dumpScreenshot();
}

void TreeViewDialog::dumpScreenshotData(const QByteArray &data)
{
    if (data.size() > 0)
    {
        paintedWidget->setImageData(data);
    }
}

MyPushButton::MyPushButton(QWidget* parent)
    : QPushButton(parent)
{
}

MyPushButton::MyPushButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent)
{
}

void MyPushButton::mousePressEvent(QMouseEvent* event)
{
    qDebug() << Q_FUNC_INFO << event;

    if (event->modifiers() == Qt::ShiftModifier)
    {
        emit shiftClicked();
    }
    else
    {
        QPushButton::mousePressEvent(event);
    }
}
