/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionSomething;
    QAction *actionHere;
    QAction *action_advanced_view;
    QWidget *centralwidget;
    QSplitter *splitter;
    QTreeView *containers_view;
    QTreeView *contents_view;
    QTreeView *attributes_view;
    QMenuBar *menubar;
    QMenu *menubar_new;
    QMenu *menuEdit;
    QMenu *menuView;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1307, 795);
        actionSomething = new QAction(MainWindow);
        actionSomething->setObjectName(QString::fromUtf8("actionSomething"));
        actionHere = new QAction(MainWindow);
        actionHere->setObjectName(QString::fromUtf8("actionHere"));
        action_advanced_view = new QAction(MainWindow);
        action_advanced_view->setObjectName(QString::fromUtf8("action_advanced_view"));
        action_advanced_view->setCheckable(true);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setGeometry(QRect(0, 0, 1301, 591));
        splitter->setOrientation(Qt::Horizontal);
        containers_view = new QTreeView(splitter);
        containers_view->setObjectName(QString::fromUtf8("containers_view"));
        containers_view->setContextMenuPolicy(Qt::CustomContextMenu);
        containers_view->setAcceptDrops(true);
        containers_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        containers_view->setDragDropMode(QAbstractItemView::DragDrop);
        containers_view->setRootIsDecorated(true);
        containers_view->setItemsExpandable(true);
        containers_view->setExpandsOnDoubleClick(true);
        splitter->addWidget(containers_view);
        containers_view->header()->setVisible(true);
        contents_view = new QTreeView(splitter);
        contents_view->setObjectName(QString::fromUtf8("contents_view"));
        contents_view->setContextMenuPolicy(Qt::CustomContextMenu);
        contents_view->setAcceptDrops(true);
        contents_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        contents_view->setDragDropMode(QAbstractItemView::DragDrop);
        contents_view->setSelectionMode(QAbstractItemView::SingleSelection);
        contents_view->setRootIsDecorated(false);
        contents_view->setItemsExpandable(false);
        contents_view->setExpandsOnDoubleClick(false);
        splitter->addWidget(contents_view);
        contents_view->header()->setVisible(true);
        attributes_view = new QTreeView(splitter);
        attributes_view->setObjectName(QString::fromUtf8("attributes_view"));
        attributes_view->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);
        attributes_view->setSelectionMode(QAbstractItemView::NoSelection);
        attributes_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        splitter->addWidget(attributes_view);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1307, 27));
        menubar_new = new QMenu(menubar);
        menubar_new->setObjectName(QString::fromUtf8("menubar_new"));
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menubar_new->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuView->menuAction());
        menuEdit->addAction(actionSomething);
        menuEdit->addAction(actionHere);
        menuView->addAction(action_advanced_view);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        actionSomething->setText(QApplication::translate("MainWindow", "Something", nullptr));
        actionHere->setText(QApplication::translate("MainWindow", "Here", nullptr));
        action_advanced_view->setText(QApplication::translate("MainWindow", "Advanced view", nullptr));
        menubar_new->setTitle(QApplication::translate("MainWindow", "New", nullptr));
        menuEdit->setTitle(QApplication::translate("MainWindow", "Edit", nullptr));
        menuView->setTitle(QApplication::translate("MainWindow", "View", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
