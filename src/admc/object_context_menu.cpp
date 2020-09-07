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

#include "object_context_menu.h"
#include "ad_interface.h"
#include "confirmation_dialog.h"
#include "select_dialog.h"
#include "rename_dialog.h"
#include "utils.h"
#include "password_dialog.h"
#include "create_dialog.h"
#include "settings.h"
#include "details_widget.h"

#include <QString>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QDir>
#include <QPoint>
#include <QModelIndex>
#include <QAbstractItemView>

void force_reload_attributes_and_diff(const QString &dn);

// Open this context menu when view requests one
void open_object_context_menu_from_view(QAbstractItemView *view, int dn_column) {
    QObject::connect(
        view, &QWidget::customContextMenuRequested,
        [=]
        (const QPoint pos) {
            const QModelIndex base_index = view->indexAt(pos);

            if (!base_index.isValid()) {
                return;
            }

            const QPoint global_pos = view->mapToGlobal(pos);

            const QModelIndex index = convert_to_source(base_index);
            const QString dn = get_dn_from_index(index, dn_column);

            ObjectContextMenu::open(global_pos, dn);
        });
}

void ObjectContextMenu::open(const QPoint &global_pos, const QString &dn) {
    auto context_menu = new ObjectContextMenu(dn);
    context_menu->exec(global_pos);
}

ObjectContextMenu::ObjectContextMenu(const QString &dn)
: QMenu()
{
    setAttribute(Qt::WA_DeleteOnClose);

    // TODO: QAction *action_to_show_menu_at = 
    addAction(tr("Details"), [this, dn]() {
        DetailsWidget::instance()->reload(dn);
    });

    addAction(tr("Delete"), [this, dn]() {
        delete_object(dn);
    });
    addAction(tr("Rename"), [this, dn]() {
        auto rename_dialog = new RenameDialog(dn);
        rename_dialog->open();
    });

    QMenu *submenu_new = addMenu("New");
    for (int i = 0; i < CreateType_COUNT; i++) {
        const CreateType type = (CreateType) i;
        const QString object_string = create_type_to_string(type);

        submenu_new->addAction(object_string, [dn, type]() {
            const auto create_dialog = new CreateDialog(dn, type);
            create_dialog->open();
        });
    }

    QAction *move_action = addAction(tr("Move"));
    connect(
        move_action, &QAction::triggered,
        [this, dn]() {
            move(dn);
        });

    const bool is_policy = AdInterface::instance()->is_policy(dn); 
    const bool is_user = AdInterface::instance()->is_user(dn); 


    if (is_policy) {
    printf("ispol\n");
        addAction(tr("Edit Policy"), [this, dn]() {
            edit_policy(dn);
        });
    } else {
    printf("notspol\n");

    }

    if (is_user) {
        QAction *add_to_group_action = addAction(tr("Add to group"));
        connect(
            add_to_group_action, &QAction::triggered,
            [this, dn]() {
                add_to_group(dn);
            });

        addAction(tr("Reset password"), [dn]() {
            const auto password_dialog = new PasswordDialog(dn);
            password_dialog->open();
        });

        const bool disabled = AdInterface::instance()->user_get_account_option(dn, AccountOption_Disabled);
        QString disable_text;
        if (disabled) {
            disable_text = tr("Enable account");
        } else {
            disable_text = tr("Disable account");
        }
        addAction(disable_text, [this, dn, disabled]() {
            AdInterface::instance()->user_set_account_option(dn, AccountOption_Disabled, !disabled);
        });
    }

    const bool dev_mode = Settings::instance()->get_bool(BoolSetting_DevMode);
    if (dev_mode) {
        addAction(tr("Force reload attributes and diff"), [dn]() {
            force_reload_attributes_and_diff(dn);
        });
    }
}

void ObjectContextMenu::delete_object(const QString &dn) {
    const QString name = AdInterface::instance()->attribute_get(dn, ATTRIBUTE_NAME);
    const QString text = QString(tr("Are you sure you want to delete \"%1\"?")).arg(name);
    const bool confirmed = confirmation_dialog(text, this);

    if (confirmed) {
        AdInterface::instance()->object_delete(dn);
    }    
}

void ObjectContextMenu::edit_policy(const QString &dn) {
    // Start policy edit process
    const auto process = new QProcess();

    const QString path = AdInterface::instance()->attribute_get(dn, "gPCFileSysPath");

    const QString program_name = "/home/kevl/admc/gpgui/gpgui";

    QStringList args = {"-p", path};

    qint64 pid;
    const bool start_success = process->startDetached(program_name, args, QString(), &pid);

    printf("edit_policy\n");
    printf("path=%s\n", qPrintable(path));
    printf("pid=%lld\n", pid);
    printf("start_success=%d\n", start_success);
}

void ObjectContextMenu::move(const QString &dn) {
    // TODO: somehow formalize "class X can only be moved to X,Y,Z..." better
    const bool is_container = AdInterface::instance()->is_container(dn);
    QList<QString> classes;
    if (is_container) {
        classes = {CLASS_CONTAINER};
    } else {
        classes = {CLASS_CONTAINER, CLASS_OU};
    }

    const QList<QString> selected_objects = SelectDialog::open(classes);

    if (selected_objects.size() == 1) {
        const QString container = selected_objects[0];

        AdInterface::instance()->object_move(dn, container);
    }
}

void ObjectContextMenu::add_to_group(const QString &dn) {
    const QList<QString> classes = {CLASS_GROUP};
    const QList<QString> selected_objects = SelectDialog::open(classes, SelectDialogMultiSelection_Yes);

    if (selected_objects.size() > 0) {
        for (auto group : selected_objects) {
            AdInterface::instance()->group_add_user(group, dn);
        }
    }
}

void force_reload_attributes_and_diff(const QString &dn) {

    QMap<QString, QList<QString>> old_attributes = AdInterface::instance()->get_all_attributes(dn);

    AdInterface::instance()->update_cache({dn});

    QList<QString> changes;

    QMap<QString, QList<QString>> new_attributes = AdInterface::instance()->get_all_attributes(dn);

    for (auto &attribute : old_attributes.keys()) {
        const QString old_value = old_attributes[attribute][0];

        if (new_attributes.contains(attribute)) {
            const QString new_value = new_attributes[attribute][0];
            
            if (new_value != old_value) {
                changes.append(QString("\"%1\": \"%2\"->\"%3\"").arg(attribute, old_value, new_value));
            }
        } else {
            changes.append(QString("\"%1\": \"%2\"->\"unset\"").arg(attribute, old_value));
        }
    }

    for (auto &attribute : old_attributes.keys()) {
        if (!old_attributes.contains(attribute)) {
            const QString new_value = new_attributes[attribute][0];
            changes.append(QString("\"%1\": \"unset\"->\"%2\"").arg(attribute, new_value));
        }
    }

    if (!changes.isEmpty()) {
        const QString name = AdInterface::instance()->attribute_get(dn, ATTRIBUTE_NAME);

        printf("Attributes of object \"%s\" changed:\n", qPrintable(name));

        for (auto change : changes) {
            printf("%s\n", qPrintable(change));
        }
    }
}