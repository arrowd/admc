/*
 * ADMC - AD Management Center
 *
 * Copyright (C) 2020-2021 BaseALT Ltd.
 * Copyright (C) 2020-2021 Dmitry Degtyarev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class ConsoleWidget;
class ObjectImpl;
class FilterDialog;
class QAction;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_show_non_containers();
    void on_dev_mode();
    void on_advanced_features();
    void on_filter_dialog_accepted();
    void on_log_searches_changed();
    void on_connect_options_dialog_accepted();

private:
    ConsoleWidget *console;
    ObjectImpl *object_impl;
    FilterDialog *filter_dialog;
    
    QAction *connect_action;
    QAction *open_filter_action;
    QAction *dev_mode_action;
    QAction *show_noncontainers_action;
    QAction *advanced_features_action;

    void connect_to_server();
    void refresh_object_tree();
};

#endif /* MAIN_WINDOW_H */
