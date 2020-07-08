/*
 * ADMC - AD Management Center
 *
 * Copyright (C) 2020 BaseALT Ltd.
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

#ifndef CONTENTS_WIDGET_H
#define CONTENTS_WIDGET_H

#include "ad_interface.h"
#include "entry_model.h"

#include <QWidget>
#include <QString>

class ContainersWidget;
class QStandardItem;
class EntryContextMenu;
class QTreeView;
class ContentsModel;

// Shows name, category and description of children of entry selected in containers view
class ContentsWidget final : public QWidget {
Q_OBJECT

public:
    ContentsWidget(ContainersWidget *containers_widget, EntryContextMenu *entry_context_menu, QWidget *parent);

signals:
    void clicked_dn(const QString &dn);

private slots:
    void on_containers_selected_changed(const QString &dn);
    void on_ad_modified();

private:
    ContentsModel *model = nullptr;
    QTreeView *view = nullptr;

};

class ContentsModel final : public EntryModel {
Q_OBJECT

public:
    // TODO: make this private again, will need to move slots into model?
    QString target_dn = "";

    ContentsModel(QObject *parent);

    void change_target(const QString &dn);

private:

    void load_row(QList<QStandardItem *> row, const QString &dn);
    void make_new_row(QStandardItem *parent, const QString &dn);
};

#endif /* CONTENTS_WIDGET_H */
