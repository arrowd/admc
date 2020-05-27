
#ifndef CONTENTS_WIDGET_H
#define CONTENTS_WIDGET_H

#include "entry_widget.h"

// Shows name, category and description of children of entry selected in containers view
class ContentsWidget final : public EntryWidget {
Q_OBJECT

public:
    ContentsWidget(AdModel *model);

public slots:
    void on_selected_container_changed(const QModelIndex &source_index);

private:
    
};

#endif /* CONTENTS_WIDGET_H */
