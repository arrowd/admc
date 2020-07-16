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

#ifndef OBJECT_MODEL_H
#define OBJECT_MODEL_H

#include <QStandardItemModel>
#include <QList>

class QMimeData;
class QModelIndex;
class QString;

// Model for objects
// Requires at least a DN column
// Implements drag/drop of objects using their DN's
class ObjectModel : public QStandardItemModel {
Q_OBJECT

public:
    const int dn_column;
    
    explicit ObjectModel(int column_count, int dn_column_in, QObject *parent);

    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    QList<QStandardItem *> find_row(const QString &dn);
    QStandardItem *find_item(const QString &dn, int col);

};

#endif /* OBJECT_MODEL_H */