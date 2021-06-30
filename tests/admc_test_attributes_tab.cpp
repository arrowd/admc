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

#include "admc_test_attributes_tab.h"

#include "editors/multi_editor.h"
#include "editors/string_editor.h"

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QString>
#include <QTreeView>
#include <QVBoxLayout>

// NOTE: can't be set because order is important. Read only
// has to be set first to enable other filters.
const QList<AttributeFilter> all_filters = []() {
    QList<AttributeFilter> out;

    for (int i = 0; i < AttributeFilter_COUNT; i++) {
        out.append((AttributeFilter) i);
    }

    return out;
}();

void ADMCTestAttributesTab::init() {
    ADMCTest::init();

    attributes_tab = new AttributesTab();

    parent_widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(parent_widget, 1000));

    auto layout = new QVBoxLayout();
    parent_widget->setLayout(layout);
    layout->addWidget(attributes_tab);

    view = attributes_tab->findChild<QTreeView *>();
    QVERIFY(view != nullptr);

    model = attributes_tab->findChild<QStandardItemModel *>();
    QVERIFY(model != nullptr);

    proxy = attributes_tab->findChild<QSortFilterProxyModel *>();
    QVERIFY(proxy != nullptr);

    filter_button = attributes_tab->findChild<QPushButton *>("filter_button");
    QVERIFY(filter_button != nullptr);

    edit_button = attributes_tab->findChild<QPushButton *>("edit_button");
    QVERIFY(filter_button != nullptr);

    // Create test user
    const QString name = TEST_USER;
    dn = test_object_dn(name, CLASS_USER);
    const bool create_success = ad.object_add(dn, CLASS_USER);
    QVERIFY(create_success);

    // Load it into the tab
    const AdObject object = ad.search_object(dn);
    attributes_tab->load(ad, object);
}

void ADMCTestAttributesTab::load() {
    // Check that some attributes loaded correctly
    auto check_value = [&](const QString &name, const QString &value) {
        const QList<QStandardItem *> item_list = model->findItems(name);

        if (item_list.size() != 1) {
            return false;
        }

        QStandardItem *name_item = item_list[0];
        QStandardItem *value_item = model->item(name_item->row(), 1);
        const QString &this_value = value_item->text();
        const bool value_is_correct = (this_value == value);

        return value_is_correct;
    };

    check_value("name", TEST_USER);
    check_value("distinguishedName", dn);
}

// Test filters by checking that affected attributes are
// visible/hidden when their filters are checked/unchecked
void ADMCTestAttributesTab::filter() {
    auto test_filter = [&](const AttributeFilter filter, const QString &attribute) {
        auto check_filtering = [&](const bool should_be_visible) {
            const QList<QStandardItem *> item_list = model->findItems(attribute);
            QVERIFY2((item_list.size() == 1), qPrintable(QString("Failed to find attribute %1)").arg(attribute)));

            QStandardItem *item = item_list[0];
            const QModelIndex index = item->index();
            const QModelIndex proxy_index = proxy->mapFromSource(index);
            const bool is_visible = proxy_index.isValid();

            const bool correct_filtering = (is_visible == should_be_visible);
            QVERIFY2(correct_filtering, qPrintable(QString("filter = %1, attribute = %2, is_visible = %3, should_be_visible = %4").arg(filter).arg(attribute).arg(is_visible).arg(should_be_visible)));
        };

        set_filter({filter}, Qt::Checked);
        check_filtering(true);
        set_filter({filter}, Qt::Unchecked);
        check_filtering(false);
        set_filter({filter}, Qt::Checked);
    };

    set_filter(all_filters, Qt::Checked);

    test_filter(AttributeFilter_Unset, ATTRIBUTE_HOME_PHONE);
    test_filter(AttributeFilter_ReadOnly, ATTRIBUTE_OBJECT_GUID);
    test_filter(AttributeFilter_SystemOnly, ATTRIBUTE_WHEN_CREATED);
    test_filter(AttributeFilter_Constructed, "allowedAttributes");
    test_filter(AttributeFilter_Backlink, ATTRIBUTE_MEMBER_OF);
    test_filter(AttributeFilter_Mandatory, "instanceType");
    test_filter(AttributeFilter_Optional, ATTRIBUTE_COUNTRY_CODE);

    // NOTE: read only also needs to affect these read only
    // attributes
    test_filter(AttributeFilter_ReadOnly, "allowedAttributes");
    test_filter(AttributeFilter_ReadOnly, ATTRIBUTE_WHEN_CREATED);
    test_filter(AttributeFilter_ReadOnly, ATTRIBUTE_MEMBER_OF);

    set_filter(all_filters, Qt::Checked);
}

// Change description attribute, apply and then confirm
// changes
void ADMCTestAttributesTab::apply() {
    const QString correct_value = "test description";

    navigate_until_object(view, "description", Qt::DisplayRole);
    edit_button->click();

    auto multi_editor = attributes_tab->findChild<MultiEditor *>();
    QVERIFY(multi_editor != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(multi_editor, 1000));

    auto add_button = multi_editor->findChild<QPushButton *>("add_button");
    QVERIFY(add_button != nullptr);
    add_button->click();

    auto string_editor = multi_editor->findChild<StringEditor *>();
    QVERIFY(string_editor != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(string_editor, 1000));

    auto string_editor_edit = string_editor->findChild<QLineEdit *>();
    QVERIFY(string_editor_edit != nullptr);
    string_editor_edit->setText(correct_value);

    string_editor->accept();
    multi_editor->accept();

    attributes_tab->apply(ad, dn);

    const AdObject object = ad.search_object(dn);
    const QString description_value = object.get_string(ATTRIBUTE_DESCRIPTION);
    QVERIFY(description_value == correct_value);
}

void ADMCTestAttributesTab::set_filter(const QList<AttributeFilter> &filter_list, const Qt::CheckState state) {
    filter_button->click();

    auto filter_dialog = attributes_tab->findChild<QDialog *>();
    QVERIFY(filter_dialog != nullptr);

    QVERIFY(QTest::qWaitForWindowExposed(filter_dialog, 1000));

    for (const AttributeFilter &filter : filter_list) {
        QCheckBox *checkbox = filter_dialog->findChild<QCheckBox *>(QString::number(filter));
        QVERIFY(checkbox != nullptr);
        checkbox->setCheckState(state);
    }

    filter_dialog->accept();

    // NOTE: manually delete because normally it's deleted
    // lazily which is a problem when we need to open this
    // dialog multiple times
    delete filter_dialog;

    QVERIFY(QTest::qWaitForWindowExposed(attributes_tab, 1000));
}

QTEST_MAIN(ADMCTestAttributesTab)
