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

#ifndef FILTER_CUSTOM_DIALOG_H
#define FILTER_CUSTOM_DIALOG_H

/**
 * Dialog that's opened when "Custom" button is pressed in
 * filter dialog. Contains a filter widget which is used to
 * create a filter.
 */

#include <QDialog>

class FilterWidget;
class AdConfig;

class FilterCustomDialog final : public QDialog {
    Q_OBJECT

public:
    FilterCustomDialog(AdConfig *adconfig, QWidget *parent);

    QVariant save_state() const;
    void restore_state(const QVariant &state);

    QString get_filter() const;

private:
    FilterWidget *filter_widget;
    
};

#endif /* FILTER_CUSTOM_DIALOG_H */
