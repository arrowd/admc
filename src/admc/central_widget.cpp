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

#include "central_widget.h"

#include "console_types/object.h"
#include "console_types/policy.h"
#include "console_types/query.h"
#include "utils.h"
#include "adldap.h"
#include "properties_dialog.h"
#include "globals.h"
#include "settings.h"
#include "filter_dialog.h"
#include "filter_widget/filter_widget.h"
#include "object_actions.h"
#include "status.h"
#include "rename_dialog.h"
#include "create_dialog.h"
#include "create_policy_dialog.h"
#include "create_query_dialog.h"
#include "create_query_folder_dialog.h"
#include "move_dialog.h"
#include "find_dialog.h"
#include "password_dialog.h"
#include "rename_policy_dialog.h"
#include "select_dialog.h"
#include "console_widget/console_widget.h"
#include "console_widget/results_view.h"
#include "editors/multi_editor.h"
#include "gplink.h"
#include "policy_results_widget.h"

#include <QDebug>
#include <QAbstractItemView>
#include <QVBoxLayout>
#include <QSplitter>
#include <QDebug>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QApplication>
#include <QTreeWidget>
#include <QStack>
#include <QMenu>
#include <QLabel>
#include <QSortFilterProxyModel>

enum DropType {
    DropType_Move,
    DropType_AddToGroup,
    DropType_None
};

DropType get_object_drop_type(const QModelIndex &dropped, const QModelIndex &target);

CentralWidget::CentralWidget()
: QWidget()
{
    object_actions = new ObjectActions(this);

    auto create_policy_action = new QAction(tr("New policy"), this);
    auto add_link_action = new QAction(tr("Add link"), this);
    auto rename_policy_action = new QAction(tr("Rename"), this);
    auto delete_policy_action = new QAction(tr("Delete"), this);

    // NOTE: create policy action is not added to list
    // because it is shown for the policies container, not
    // for gpo's themselves.

    item_actions[ItemType_PoliciesRoot] = {
        create_policy_action,
    };
    
    // NOTE: add policy actions to this list so that they
    // are processed
    item_actions[ItemType_Policy] = {
        add_link_action,
        rename_policy_action,
        delete_policy_action,
    };

    auto delete_query_item_or_folder_action = new QAction(tr("Delete"), this);

    auto new_query_folder_action = new QAction(tr("New folder"), this);
    auto new_query_action = new QAction(tr("New query"), this);
    item_actions[ItemType_QueryFolder] = {
        new_query_folder_action,
        new_query_action,
        delete_query_item_or_folder_action,
    };

    item_actions[ItemType_QueryItem] = {
        delete_query_item_or_folder_action,
    };

    item_actions[ItemType_QueriesRoot] = {
        new_query_folder_action,
        new_query_action,
    };

    open_filter_action = new QAction(tr("&Filter objects"), this);
    dev_mode_action = new QAction(tr("Dev mode"), this);
    show_noncontainers_action = new QAction(tr("&Show non-container objects in Console tree"), this);

    filter_dialog = nullptr;
    open_filter_action->setEnabled(false);

    console = new ConsoleWidget();

    object_results = new ResultsView(this);
    // TODO: not sure how to do this. View headers dont even
    // have sections until their models are loaded.
    // g_settings->setup_header_state(object_results->header(),
    // VariantSetting_ResultsHeader);

    auto policy_container_results = new ResultsView(this);
    policy_container_results->detail_view()->header()->setDefaultSectionSize(200);
    policy_container_results_id = console->register_results(policy_container_results, policy_model_header_labels(), policy_model_default_columns());
    
    policy_results_widget = new PolicyResultsWidget();
    policy_results_id = console->register_results(policy_results_widget);

    auto query_results = new ResultsView(this);
    query_results->detail_view()->header()->setDefaultSectionSize(200);
    query_folder_results_id = console->register_results(query_results, query_folder_header_labels(), query_folder_default_columns());
    
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(console);

    // Refresh head when settings affecting the filter
    // change. This reloads the model with an updated filter
    const BoolSettingSignal *advanced_features = g_settings->get_bool_signal(BoolSetting_AdvancedFeatures);
    connect(
        advanced_features, &BoolSettingSignal::changed,
        this, &CentralWidget::refresh_head);

    const BoolSettingSignal *show_non_containers = g_settings->get_bool_signal(BoolSetting_ShowNonContainersInConsoleTree);
    connect(
        show_non_containers, &BoolSettingSignal::changed,
        this, &CentralWidget::refresh_head);

    const BoolSettingSignal *dev_mode_signal = g_settings->get_bool_signal(BoolSetting_DevMode);
    connect(
        dev_mode_signal, &BoolSettingSignal::changed,
        this, &CentralWidget::refresh_head);

    g_settings->connect_toggle_widget(console->get_scope_view(), BoolSetting_ShowConsoleTree);
    g_settings->connect_toggle_widget(console->get_description_bar(), BoolSetting_ShowResultsHeader);

    g_settings->connect_action_to_bool_setting(dev_mode_action, BoolSetting_DevMode);
    g_settings->connect_action_to_bool_setting(show_noncontainers_action, BoolSetting_ShowNonContainersInConsoleTree);

    connect(
        open_filter_action, &QAction::triggered,
        this, &CentralWidget::open_filter);

    connect(
        object_actions->get(ObjectAction_NewUser), &QAction::triggered,
        this, &CentralWidget::create_user);
    connect(
        object_actions->get(ObjectAction_NewComputer), &QAction::triggered,
        this, &CentralWidget::create_computer);
    connect(
        object_actions->get(ObjectAction_NewOU), &QAction::triggered,
        this, &CentralWidget::create_ou);
    connect(
        object_actions->get(ObjectAction_NewGroup), &QAction::triggered,
        this, &CentralWidget::create_group);
    connect(
        object_actions->get(ObjectAction_Delete), &QAction::triggered,
        this, &CentralWidget::delete_objects);
    connect(
        object_actions->get(ObjectAction_Rename), &QAction::triggered,
        this, &CentralWidget::rename);
    connect(
        object_actions->get(ObjectAction_Move), &QAction::triggered,
        this, &CentralWidget::move);
    connect(
        object_actions->get(ObjectAction_AddToGroup), &QAction::triggered,
        this, &CentralWidget::add_to_group);
    connect(
        object_actions->get(ObjectAction_Enable), &QAction::triggered,
        this, &CentralWidget::enable);
    connect(
        object_actions->get(ObjectAction_Disable), &QAction::triggered,
        this, &CentralWidget::disable);
    connect(
        object_actions->get(ObjectAction_ResetPassword), &QAction::triggered,
        this, &CentralWidget::reset_password);
    connect(
        object_actions->get(ObjectAction_Find), &QAction::triggered,
        this, &CentralWidget::find);
    connect(
        object_actions->get(ObjectAction_EditUpnSuffixes), &QAction::triggered,
        this, &CentralWidget::edit_upn_suffixes);

    connect(
        create_policy_action, &QAction::triggered,
        this, &CentralWidget::create_policy);
    connect(
        add_link_action, &QAction::triggered,
        this, &CentralWidget::add_link);
    connect(
        rename_policy_action, &QAction::triggered,
        this, &CentralWidget::rename_policy);
    connect(
        delete_policy_action, &QAction::triggered,
        this, &CentralWidget::delete_policy);

    connect(
        new_query_folder_action, &QAction::triggered,
        this, &CentralWidget::new_query_folder);
    connect(
        new_query_action, &QAction::triggered,
        this, &CentralWidget::new_query);
    connect(
        delete_query_item_or_folder_action, &QAction::triggered,
        this, &CentralWidget::delete_query_item_or_folder);

    connect(
        console, &ConsoleWidget::current_scope_item_changed,
        this, &CentralWidget::on_current_scope_changed);
    connect(
        console, &ConsoleWidget::results_count_changed,
        this, &CentralWidget::update_description_bar);
    connect(
        console, &ConsoleWidget::item_fetched,
        this, &CentralWidget::fetch_scope_node);
    connect(
        console, &ConsoleWidget::items_can_drop,
        this, &CentralWidget::on_items_can_drop);
    connect(
        console, &ConsoleWidget::items_dropped,
        this, &CentralWidget::on_items_dropped);
    connect(
        console, &ConsoleWidget::properties_requested,
        this, &CentralWidget::on_properties_requested);
    connect(
        console, &ConsoleWidget::selection_changed,
        this, &CentralWidget::update_actions_visibility);
    connect(
        console, &ConsoleWidget::context_menu,
        this, &CentralWidget::context_menu);

    update_actions_visibility();
}

void CentralWidget::go_online(AdInterface &ad) {
    // NOTE: filter dialog requires a connection to load
    // display strings from adconfig so create it here
    filter_dialog = new FilterDialog(this);
    connect(
        filter_dialog, &QDialog::accepted,
        this, &CentralWidget::refresh_head);
    open_filter_action->setEnabled(true);

    // NOTE: Header labels are from ADCONFIG, so have to get them
    // after going online
    object_results_id = console->register_results(object_results, object_model_header_labels(), object_model_default_columns());

    // Add top domain item
    const QString head_dn = g_adconfig->domain_head();
    const AdObject head_object = ad.search_object(head_dn);

    QStandardItem *item = console->add_scope_item(object_results_id, ScopeNodeType_Dynamic, QModelIndex());

    scope_head_index = QPersistentModelIndex(item->index());

    setup_object_scope_item(item, head_object);

    const QString domain_text =
    [&]() {
        const QString name = item->text();
        const QString host = ad.host();

        return QString("%1 [%2]").arg(name, host);
    }();
    item->setText(domain_text);

    console->set_current_scope(item->index());

    // Add top policies item
    QStandardItem *policies_item = console->add_scope_item(policy_container_results_id, ScopeNodeType_Static, QModelIndex());
    policies_item->setText(tr("Group Policy Objects"));
    policies_index = QPersistentModelIndex(policies_item->index());
    policies_item->setDragEnabled(false);
    policies_item->setIcon(QIcon::fromTheme("folder"));
    policies_item->setData(ItemType_PoliciesRoot, ConsoleRole_Type);

    // Load policies items
    const QList<QString> policy_search_attributes = policy_model_search_attributes();
    const QString policy_search_filter = filter_CONDITION(Condition_Equals, ATTRIBUTE_OBJECT_CLASS, CLASS_GP_CONTAINER);

    const QHash<QString, AdObject> policy_search_results = ad.search(policy_search_filter, policy_search_attributes, SearchScope_All);

    for (const AdObject &object : policy_search_results.values()) {
        console_add_policy(console, policies_index, object);
    }

    // Add top queries item
    QStandardItem *queries_item = console->add_scope_item(query_folder_results_id, ScopeNodeType_Static, QModelIndex());
    queries_index = QPersistentModelIndex(queries_item->index());
    queries_item->setText(tr("Saved Queries"));
    queries_item->setIcon(QIcon::fromTheme("folder"));
    queries_item->setData(ItemType_QueriesRoot, ConsoleRole_Type);

    // Load queries item
    const QHash<QString, QVariant> folders_map = g_settings->get_variant(VariantSetting_QueryFolders).toHash();
    const QHash<QString, QVariant> info_map = g_settings->get_variant(VariantSetting_QueryInfo).toHash();

    QStack<QModelIndex> query_stack;
    query_stack.append(queries_index);
    while (!query_stack.isEmpty()) {
        const QModelIndex index = query_stack.pop();
        const QString path = query_folder_path(index);

        // Go through children and add them as folders or
        // query items
        const QList<QString> children = folders_map[path].toStringList();
        for (const QString &child_path : children) {
            const QHash<QString, QVariant> info = info_map[child_path].toHash();
            const bool is_query_item = info["is_query_item"].toBool();

            if (is_query_item) {
                // Query item
                const QString description = info["description"].toString();
                const QString filter = info["filter"].toString();
                const QString search_base = info["search_base"].toString();
                const QString name = path_to_name(child_path);
                add_query_item(console, name, description, filter, search_base, index);
            } else {
                // Query folder
                const QString name = path_to_name(child_path);
                const QString description = info["description"].toString();
                const QModelIndex child_scope_index = add_query_folder(console, name, description, index);

                query_stack.append(child_scope_index);
            }
        }
    }

    console->sort_scope();
}

void CentralWidget::open_filter() {
    if (filter_dialog != nullptr) {
        filter_dialog->open();
    }
}

void CentralWidget::delete_objects() {
    const QHash<QString, QPersistentModelIndex> selected = get_selected_dns_and_indexes();
    const QList<QString> deleted_objects = object_delete(selected.keys(), this);

    console_delete_objects(console, deleted_objects);
}

void CentralWidget::on_properties_requested() {
    const QHash<QString, QPersistentModelIndex> targets = get_selected_dns_and_indexes();
    if (targets.size() != 1) {
        return;
    }

    const QString target = targets.keys()[0];

    PropertiesDialog *dialog = PropertiesDialog::open_for_target(target);

    connect(
        dialog, &PropertiesDialog::applied,
        [=]() {
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            const AdObject updated_object = ad.search_object(target);
            console_update_object(console, updated_object);

            update_actions_visibility();
        });
}

void CentralWidget::rename() {
    const QHash<QString, QPersistentModelIndex> targets = get_selected_dns_and_indexes();

    auto dialog = new RenameDialog(targets.keys(), this);
    dialog->open();

    connect(
        dialog, &RenameDialog::accepted,
        [=]() {
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            const QString old_dn = targets.keys()[0];
            const QString new_dn = dialog->get_new_dn();
            const QString parent_dn = dn_get_parent(old_dn);
            console_move_objects(console, ad, {old_dn}, {new_dn}, parent_dn);
        });
}

void CentralWidget::create_helper(const QString &object_class) {
    const QHash<QString, QPersistentModelIndex> targets = get_selected_dns_and_indexes();

    auto dialog = new CreateDialog(targets.keys(), object_class, this);
    dialog->open();

    // NOTE: can't just add new object to console by adding
    // to selected index, because you can create an object
    // by using action menu of an object in a query tree.
    // Therefore need to search for parent in domain tree.
    connect(
        dialog, &CreateDialog::accepted,
        [=]() {
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            show_busy_indicator();

            const QString parent_dn = targets.keys()[0];
            const QList<QModelIndex> search_parent = console->search_scope_by_role(ObjectRole_DN, parent_dn, ItemType_DomainObject);

            if (search_parent.isEmpty()) {
                hide_busy_indicator();
                return;
            }

            const QModelIndex scope_parent_index = search_parent[0];
            const QString created_dn = dialog->get_created_dn();
            console_add_objects(console, ad, {created_dn}, scope_parent_index);

            console->sort_scope();

            hide_busy_indicator();
        });
}

void CentralWidget::move() {
    const QHash<QString, QPersistentModelIndex> targets = get_selected_dns_and_indexes();

    auto dialog = new MoveDialog(targets.keys(), this);
    dialog->open();

    connect(
        dialog, &QDialog::accepted,
        [=]() {
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            const QList<QString> old_dn_list = dialog->get_moved_objects();
            const QString new_parent_dn = dialog->get_selected();
            console_move_objects(console, ad, old_dn_list, new_parent_dn);

            console->sort_scope();
        });
}

void CentralWidget::add_to_group() {
    const QList<QString> targets = get_selected_dns();
    object_add_to_group(targets, this);
}

void CentralWidget::enable() {
    enable_disable_helper(false);
}

void CentralWidget::disable() {
    enable_disable_helper(true);
}

void CentralWidget::find() {
    const QList<QString> targets = get_selected_dns();

    if (targets.size() != 1) {
        return;
    }

    const QString target = targets[0];

    auto find_dialog = new FindDialog(filter_classes, target, this);
    find_dialog->open();
}

void CentralWidget::reset_password() {
    const QList<QString> targets = get_selected_dns();
    const auto password_dialog = new PasswordDialog(targets, this);
    password_dialog->open();
}

void CentralWidget::create_user() {
    create_helper(CLASS_USER);
}

void CentralWidget::create_computer() {
    create_helper(CLASS_COMPUTER);
}

void CentralWidget::create_ou() {
    create_helper(CLASS_OU);
}

void CentralWidget::create_group() {
    create_helper(CLASS_GROUP);
}

void CentralWidget::edit_upn_suffixes() {
    AdInterface ad;
    if (ad_failed(ad)) {
        return;
    }

    // Open editor for upn suffixes attribute of partitions
    // object
    const QString partitions_dn = g_adconfig->partitions_dn();
    const AdObject partitions_object = ad.search_object(partitions_dn);
    const QList<QByteArray> current_values = partitions_object.get_values(ATTRIBUTE_UPN_SUFFIXES);

    auto editor = new MultiEditor(ATTRIBUTE_UPN_SUFFIXES, current_values, this);
    editor->open();

    // When editor is accepted, update values of upn
    // suffixes
    connect(editor, &QDialog::accepted,
        [this, editor, partitions_dn]() {
            AdInterface ad2;
            if (ad_failed(ad2)) {
                return;
            }

            const QList<QByteArray> new_values = editor->get_new_values();

            ad2.attribute_replace_values(partitions_dn, ATTRIBUTE_UPN_SUFFIXES, new_values);
            g_status()->display_ad_messages(ad2, this);
        });
}

void CentralWidget::create_policy() {
    // TODO: implement using ad.create_gpo() (which is
    // unfinished)

    auto dialog = new CreatePolicyDialog(this);

    connect(
        dialog, &QDialog::accepted,
        [this, dialog]() {
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            const QString dn = dialog->get_created_dn();

            const QList<QString> search_attributes = policy_model_search_attributes();
            const QHash<QString, AdObject> search_results = ad.search(QString(), search_attributes, SearchScope_Object, dn);
            const AdObject object = search_results[dn];

            console_add_policy(console, policies_index, object);

            // NOTE: not adding policy object to the domain
            // tree, but i think it's ok?
        });

    dialog->open();
}

void CentralWidget::add_link() {
    const QList<QModelIndex> selected = console->get_selected_items();
    if (selected.size() == 0) {
        return;
    }

    auto dialog = new SelectDialog({CLASS_OU}, SelectDialogMultiSelection_Yes, this);

    QObject::connect(
        dialog, &SelectDialog::accepted,
        [=]() {           
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            show_busy_indicator();

            const QList<QString> gpos =
            [selected]() {
                QList<QString> out;

                for (const QModelIndex &index : selected) {
                    const QString gpo = index.data(PolicyRole_DN).toString();
                    out.append(gpo);
                }

                return out;
            }();

            const QList<QString> ou_list = dialog->get_selected();

            for (const QString &ou_dn : ou_list) {
                const QHash<QString, AdObject> results = ad.search(QString(), {ATTRIBUTE_GPLINK}, SearchScope_Object, ou_dn);
                const AdObject ou_object = results[ou_dn];
                const QString gplink_string = ou_object.get_string(ATTRIBUTE_GPLINK);
                Gplink gplink = Gplink(gplink_string);

                for (const QString &gpo : gpos) {
                    gplink.add(gpo);
                }

                ad.attribute_replace_string(ou_dn, ATTRIBUTE_GPLINK, gplink.to_string());
            }

            const QModelIndex current_scope = console->get_current_scope_item();
            policy_results_widget->update(current_scope);

            hide_busy_indicator();

            g_status()->display_ad_messages(ad, this);
        });

    dialog->open();
}

void CentralWidget::rename_policy() {
    const QList<QModelIndex> indexes = console->get_selected_items();
    if (indexes.size() != 1) {
        return;
    }

    const QModelIndex index = indexes[0];
    const QString dn = index.data(PolicyRole_DN).toString();

    auto dialog = new RenamePolicyDialog(dn, this);
    dialog->open();

    connect(
        dialog, &RenamePolicyDialog::accepted,
        [=]() {
            AdInterface ad;
            if (ad_failed(ad)) {
                return;
            }

            const AdObject updated_object = ad.search_object(dn);
            console_update_policy(console, index, updated_object);

            console->sort_scope();
        });
}

void CentralWidget::delete_policy() {
    const QHash<QString, QPersistentModelIndex> selected = get_selected_dns_and_indexes();

    if (selected.size() == 0) {
        return;
    }

    const bool confirmed = confirmation_dialog(tr("Are you sure you want to delete this policy and all of it's links?"), this);
    if (!confirmed) {
        return;
    }

    AdInterface ad;
    if (ad_failed(ad)) {
        return;
    }

    show_busy_indicator();

    for (const QModelIndex &index : selected) {
        const QString dn = index.data(PolicyRole_DN).toString();
        const bool success = ad.object_delete(dn);

        if (success) {
            // Remove deleted policy from console
            console->delete_item(index);

            // Remove links to delete policy
            const QString filter = filter_CONDITION(Condition_Contains, ATTRIBUTE_GPLINK, dn);
            const QList<QString> search_attributes = {
                ATTRIBUTE_GPLINK,
            };
            const QHash<QString, AdObject> search_results = ad.search(filter, search_attributes, SearchScope_All);
            for (const AdObject &object : search_results.values()) {
                const QString gplink_string = object.get_string(ATTRIBUTE_GPLINK);
                Gplink gplink = Gplink(gplink_string);
                gplink.remove(dn);

                const QString updated_gplink_string = gplink.to_string();
                ad.attribute_replace_string(object.get_dn(), ATTRIBUTE_GPLINK, updated_gplink_string);
            }
        }
    }

    hide_busy_indicator();

    g_status()->display_ad_messages(ad, this);
}

void CentralWidget::new_query_folder() {
    const QList<QModelIndex> selected_indexes = console->get_selected_items();
    
    if (selected_indexes.size() != 1) {
        return;
    }

    const QModelIndex parent_index = console->convert_to_scope_index(selected_indexes[0]);

    const QList<QString> sibling_names = get_sibling_names(parent_index);

    auto dialog = new CreateQueryFolderDialog(sibling_names, this);

    connect(
        dialog, &QDialog::accepted,
        [=]() {
            const QString name = dialog->get_name();
            const QString description = dialog->get_description();
            add_query_folder(console, name, description, parent_index);

            save_queries();
        });

    dialog->open();
}

void CentralWidget::new_query() {
    const QList<QModelIndex> selected_indexes = console->get_selected_items();
    
    if (selected_indexes.size() != 1) {
        return;
    }

    const QModelIndex parent_index = console->convert_to_scope_index(selected_indexes[0]);

    const QList<QString> sibling_names = get_sibling_names(parent_index);

    auto dialog = new CreateQueryDialog(sibling_names, this);

    connect(
        dialog, &QDialog::accepted,
        [=]() {
            const QString name = dialog->get_name();
            const QString description = dialog->get_description();
            const QString filter = dialog->get_filter();
            const QString search_base = dialog->get_search_base();
            add_query_item(console, name, description, filter, search_base, parent_index);

            save_queries();
        });

    dialog->open();
}

void CentralWidget::delete_query_item_or_folder() {
    const QList<QModelIndex> selected_indexes = console->get_selected_items();
    
    if (selected_indexes.size() != 1) {
        return;
    }

    const QModelIndex index = selected_indexes[0];
    console->delete_item(index);

    save_queries();
}

// NOTE: only check if object can be dropped if dropping a
// single object, because when dropping multiple objects it
// is ok for some objects to successfully drop and some to
// fail. For example, if you drop users together with OU's
// onto a group, users will be added to that group while OU
// will fail to drop.
void CentralWidget::on_items_can_drop(const QList<QModelIndex> &dropped_list, const QModelIndex &target, bool *ok) {
    if (dropped_list.size() != 1) {
        *ok = true;
        return;
    } else {
        const QModelIndex dropped = dropped_list[0];

        const DropType drop_type = get_object_drop_type(dropped, target);
        const bool can_drop = (drop_type != DropType_None);

        *ok = can_drop;
    }
}

void CentralWidget::on_items_dropped(const QList<QModelIndex> &dropped_list, const QModelIndex &target) {
    const QString target_dn = target.data(ObjectRole_DN).toString();

    AdInterface ad;
    if (ad_failed(ad)) {
        return;
    }

    show_busy_indicator();

    for (const QModelIndex &dropped : dropped_list) {
        const QString dropped_dn = dropped.data(ObjectRole_DN).toString();
        const DropType drop_type = get_object_drop_type(dropped, target);

        switch (drop_type) {
            case DropType_Move: {
                const bool move_success = ad.object_move(dropped_dn, 
                    target_dn);

                if (move_success) {
                    console_move_objects(console, ad, QList<QString>({dropped_dn}), target_dn);
                }

                break;
            }
            case DropType_AddToGroup: {
                ad.group_add_member(target_dn, dropped_dn);

                break;
            }
            case DropType_None: {
                break;
            }
        }
    }

    console->sort_scope();

    hide_busy_indicator();

    g_status()->display_ad_messages(ad, nullptr);
}

void CentralWidget::on_current_scope_changed() {
    const QModelIndex current_scope = console->get_current_scope_item();
    policy_results_widget->update(current_scope);

    update_description_bar();
}

void CentralWidget::refresh_head() {
    show_busy_indicator();

    console->refresh_scope(scope_head_index);

    hide_busy_indicator();
}

// TODO: currently calling this when current scope changes,
// but also need to call this when items are added/deleted
void CentralWidget::update_description_bar() {
    const QString text =
    [this]() {
        const QModelIndex current_scope = console->get_current_scope_item();
        const ItemType type = (ItemType) current_scope.data(ConsoleRole_Type).toInt();

        if (type == ItemType_DomainObject) {
            const int results_count = console->get_current_results_count();
            const QString out = tr("%n object(s)", "", results_count);

            return out;
        } else {
            return QString();
        }
    }();

    console->set_description_bar_text(text);
}

void CentralWidget::add_actions_to_action_menu(QMenu *menu) {
    object_actions->add_to_menu(menu);

    for (const QList<QAction *> actions : item_actions.values()) {
        for (QAction *action : actions) {
            menu->addAction(action);
        }
    }

    menu->addSeparator();

    console->add_actions_to_action_menu(menu);
}

void CentralWidget::add_actions_to_navigation_menu(QMenu *menu) {
    console->add_actions_to_navigation_menu(menu);
}

void CentralWidget::add_actions_to_view_menu(QMenu *menu) {
    console->add_actions_to_view_menu(menu);

    menu->addSeparator();

    menu->addAction(open_filter_action);

    menu->addAction(show_noncontainers_action);

    #ifdef QT_DEBUG
    menu->addAction(dev_mode_action);
    #endif
}

void CentralWidget::fetch_scope_node(const QModelIndex &index) {
    const ItemType type = (ItemType) index.data(ConsoleRole_Type).toInt();

    if (type == ItemType_DomainObject) {
        fetch_object(console, filter_dialog, index);
    } else if (type == ItemType_QueryItem) {
        fetch_query(console, index);
    }
}

// Determine what kind of drop type is dropping this object
// onto target. If drop type is none, then can't drop this
// object on this target.
DropType get_object_drop_type(const QModelIndex &dropped, const QModelIndex &target) {
    const bool dropped_is_target =
    [&]() {
        const QString dropped_dn = dropped.data(ObjectRole_DN).toString();
        const QString target_dn = target.data(ObjectRole_DN).toString();

        return (dropped_dn == target_dn);
    }();

    const QList<QString> dropped_classes = dropped.data(ObjectRole_ObjectClasses).toStringList();
    const QList<QString> target_classes = target.data(ObjectRole_ObjectClasses).toStringList();

    const bool dropped_is_user = dropped_classes.contains(CLASS_USER);
    const bool dropped_is_group = dropped_classes.contains(CLASS_GROUP);
    const bool target_is_group = target_classes.contains(CLASS_GROUP);

    if (dropped_is_target) {
        return DropType_None;
    } else if (dropped_is_user && target_is_group) {
        return DropType_AddToGroup;
    } else if (dropped_is_group && target_is_group) {
        return DropType_AddToGroup;
    } else {
        const QList<QString> dropped_superiors = g_adconfig->get_possible_superiors(dropped_classes);

        const bool target_is_valid_superior =
        [&]() {
            for (const auto &object_class : dropped_superiors) {
                if (target_classes.contains(object_class)) {
                    return true;
                }
            }

            return false;
        }();

        if (target_is_valid_superior) {
            return DropType_Move;
        } else {
            return DropType_None;
        }
    }
}

void CentralWidget::enable_disable_helper(const bool disabled) {
    const QHash<QString, QPersistentModelIndex> targets = get_selected_dns_and_indexes();

    show_busy_indicator();

    const QList<QString> changed_objects = object_enable_disable(targets.keys(), disabled, this);

    AdInterface ad;
    if (ad_failed(ad)) {
        return;
    }

    for (const QString &dn : changed_objects) {
        const QPersistentModelIndex index = targets[dn];

        auto update_helper =
        [=](const QPersistentModelIndex &the_index) {
            const bool is_scope = console->is_scope_item(the_index);

            if (is_scope) {
                QStandardItem *scope_item = console->get_scope_item(the_index);

                scope_item->setData(disabled, ObjectRole_AccountDisabled);
            } else {
                QList<QStandardItem *> results_row = console->get_results_row(the_index);
                results_row[0]->setData(disabled, ObjectRole_AccountDisabled);
            }
        };

        update_helper(index);

        const QPersistentModelIndex buddy = console->get_buddy(index);
        if (buddy.isValid()) {
            update_helper(buddy);
        }
    }

    update_actions_visibility();

    hide_busy_indicator();
}

// First, hide all actions, then show whichever actions are
// appropriate for current console selection
void CentralWidget::update_actions_visibility() {
    // Hide all actions
    object_actions->hide_actions();

    for (const QList<QAction *> actions : item_actions.values()) {
        for (QAction *action : actions) {
            action->setVisible(false);
        }
    }

    // Figure out what kind of types of items are selected
    const QList<QModelIndex> selected_indexes = console->get_selected_items();
    if (selected_indexes.isEmpty()) {
        return;
    }

    const QSet<ItemType> selected_types =
    [&selected_indexes]() {
        QSet<ItemType> out;

        for (const QModelIndex &index : selected_indexes) {
            const ItemType type = (ItemType) index.data(ConsoleRole_Type).toInt();
            out.insert(type);
        }

        return out;
    }();

    if (selected_types.size() != 1) {
        return;
    }

    const ItemType type = selected_types.toList()[0];

    // Show actions of selected type
    const QList<QAction *> type_actions = item_actions[type];
    for (QAction *action : type_actions) {
        action->setVisible(true);
    }

    if (type == ItemType_DomainObject) {
        object_actions->update_actions_visibility(selected_indexes);
    }
}

// Get selected indexes mapped to their DN's
QHash<QString, QPersistentModelIndex> CentralWidget::get_selected_dns_and_indexes() {
    QHash<QString, QPersistentModelIndex> out;

    const QList<QModelIndex> indexes = console->get_selected_items();
    for (const QModelIndex &index : indexes) {
        const QString dn = index.data(ObjectRole_DN).toString();
        out[dn] = QPersistentModelIndex(index);
    }

    return out;
}

QList<QString> CentralWidget::get_selected_dns() {
    const QHash<QString, QPersistentModelIndex> selected = get_selected_dns_and_indexes();

    return selected.keys();
}

// Saves current state of queries tree to settings. Should
// be called after every modication to queries tree
void CentralWidget::save_queries() {
    // folder path = {list of children}
    // data = {path => data map containing info about
    // query folder/item}
    QHash<QString, QVariant> folders;
    QHash<QString, QVariant> info_map;

    QStack<QModelIndex> stack;
    stack.append(queries_index);

    const QAbstractItemModel *model = queries_index.model();

    while (!stack.isEmpty()) {
        const QModelIndex index = stack.pop();

        // Add children to stack
        for (int i = 0; i < model->rowCount(index); i++) {
            const QModelIndex child = model->index(i, 0, index);

            stack.append(child);
        }

        // NOTE: don't save head item, it's created manually
        const bool is_query_root = !index.parent().isValid();
        if (is_query_root) {
            continue;
        }

        const QString path = query_folder_path(index);
        const QString parent_path = query_folder_path(index.parent());
        const ItemType type = (ItemType) index.data(ConsoleRole_Type).toInt();

        QList<QString> child_folders = folders[parent_path].toStringList();
        child_folders.append(path);
        folders[parent_path] = QVariant(child_folders);

        const QString description = index.data(QueryItemRole_Description).toString();

        if (type == ItemType_QueryFolder) {
            QHash<QString, QVariant> info;
            info["is_query_item"] = QVariant(false);
            info["description"] = QVariant(description);
            info_map[path] = QVariant(info);
        } else {
            const QString filter = index.data(QueryItemRole_Filter).toString();
            const QString search_base = index.data(QueryItemRole_SearchBase).toString();

            QHash<QString, QVariant> info;
            info["is_query_item"] = QVariant(true);
            info["description"] = QVariant(description);
            info["filter"] = QVariant(filter);
            info["search_base"] = QVariant(search_base);
            info_map[path] = QVariant(info);
        }
    }

    const QVariant folders_variant = QVariant(folders);
    const QVariant info_map_variant = QVariant(info_map);

    g_settings->set_variant(VariantSetting_QueryFolders, folders_variant);
    g_settings->set_variant(VariantSetting_QueryInfo, info_map_variant);
}
