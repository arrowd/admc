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

#include "members_widget.h"
#include "members_model.h"
#include "entry_context_menu.h"

#include <QTreeView>
#include <QLabel>

MembersWidget::MembersWidget(MembersModel *model, EntryContextMenu *entry_context_menu, QWidget *parent)
: EntryWidget(model, parent)
{   
    members_model = model;

    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setAcceptDrops(true);
    view->setModel(members_model);
    entry_context_menu->connect_view(view, MembersModel::Column::DN);
}

void MembersWidget::change_target(const QString &dn) {
    QModelIndex root_index = members_model->change_target(dn);

    // Set root to group index so that it is hidden
    view->setRootIndex(root_index);
}
