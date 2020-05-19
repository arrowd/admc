
#include "ad_interface.h"
#include "constants.h"

extern "C" {
#include "active_directory.h"
}

// TODO: replace C active_directory.h with C++ version

// -----------------------------------------------------------------
// FAKE STUFF
// -----------------------------------------------------------------

AdInterface ad_interface;

bool FAKE_AD = false; 

QMap<QString, QList<QString>> fake_children;
QMap<QString, QMap<QString, QList<QString>>> fake_attributes;

void fake_ad_init() {
    fake_children[HEAD_DN] = {
        QString("CN=Users,") + HEAD_DN,
        QString("CN=Computers,") + HEAD_DN,
        QString("CN=A,") + HEAD_DN,
        QString("CN=B,") + HEAD_DN,
        QString("CN=C,") + HEAD_DN,
        QString("CN=D,") + HEAD_DN,
    };

    fake_attributes[HEAD_DN] = {
        {"name", {"domain"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Container,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };

    fake_attributes[QString("CN=Users,") + HEAD_DN] = {
        {"name", {"Users"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Container,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
        {"description", {"Users's description"}},
    };

    fake_attributes[QString("CN=Computers,") + HEAD_DN] = {
        {"name", {"Computers"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Container,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
        {"description", {"Computers's description"}},
    };

    fake_attributes[QString("CN=A,") + HEAD_DN] = {
        {"name", {"A"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Container,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
        {"description", {"A's description"}},
    };

    fake_attributes[QString("CN=B,") + HEAD_DN] = {
        {"name", {"B"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Container,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
        {"description", {"B's description"}},
    };

    fake_attributes[QString("CN=C,") + HEAD_DN] = {
        {"name", {"C"}},
        {"objectClass", {"person"}},
        {"objectCategory", {"CN=Person,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };

    fake_attributes[QString("CN=D,") + HEAD_DN] = {
        {"name", {"D"}},
        {"objectClass", {"person"}},
        {"objectCategory", {"CN=Person,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"TRUE"}},
    };

    fake_children[QString("CN=B,") + HEAD_DN] = {
        QString("CN=B's child,CN=B,") + HEAD_DN
    };
    fake_attributes[QString("CN=B's child,CN=B,") + HEAD_DN] = {
        {"name", {"B's child"}},
        {"objectClass", {"person"}},
        {"objectCategory", {"CN=Person,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };
}

QList<QString> fake_load_children(const QString &dn) {
    if (!fake_children.contains(dn)) {
        // NOTE: ok to have empty children for leaves
        fake_children[dn] = QList<QString>();
    }
    
    return fake_children[dn];
}

QMap<QString, QList<QString>> fake_load_attributes(const QString &dn) {
    if (!fake_attributes.contains(dn)) {
        printf("load_attributes failed for %s, loading empty attributes\n", qPrintable(dn));
        fake_attributes[dn] = QMap<QString, QList<QString>>();
    }

    return fake_attributes[dn];
}

// NOTE: this is just for fake_create() functions
void fake_create_add_child(const QString &dn, const QString &parent) {
    if (!fake_children.contains(parent)) {
        fake_children[parent] = QList<QString>();
    }
    
    fake_children[parent].push_back(dn);
}

void fake_create_user(const QString &dn, const QString &parent, const QString &name) {
    fake_create_add_child(dn, parent);

    fake_attributes[dn] = {
        {"name", {name}},
        {"objectClass", {"user"}},
        {"objectCategory", {"CN=User,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };
}

void fake_create_computer(const QString &dn, const QString &parent, const QString &name) {
    fake_create_add_child(dn, parent);

    fake_attributes[dn] = {
        {"name", {name}},
        {"objectClass", {"computer"}},
        {"objectCategory", {"CN=Computer,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };
}

void fake_create_ou(const QString &dn, const QString &parent, const QString &name) {
    fake_create_add_child(dn, parent);

    fake_attributes[dn] = {
        {"name", {name}},
        {"objectClass", {"Organizational Unit"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Organizational-Unit,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };
}

void fake_create_group(const QString &dn, const QString &parent, const QString &name) {
    fake_create_add_child(dn, parent);

    fake_attributes[dn] = {
        {"name", {name}},
        {"objectClass", {"group"}},
        {"objectClass", {"container"}},
        {"objectCategory", {"CN=Group,CN=Schema,CN=Configuration"}},
        {"showInAdvancedViewOnly", {"FALSE"}},
    };
}

void fake_object_delete_recurse(const QString &dn) {
    if (fake_children.contains(dn)) {
        QList<QString> children = fake_children[dn];

        for (auto child : children) {
            fake_object_delete_recurse(child);
        }

        fake_children.remove(dn);
    }

    fake_attributes.remove(dn);
}

void fake_object_delete(const QString &dn) {
    fake_object_delete_recurse(dn);

    // Remove original deleted entry from parent's children list
    for (auto key : fake_children.keys()) {
        QList<QString> *children = &fake_children[key];
        
        if (children->contains(dn)) {
            int i = children->indexOf(dn);

            children->removeAt(i);
        }
    }
}

void fake_move_user(const QString &user_dn, const QString &container_dn) {
    QString user_name = extract_name_from_dn(user_dn);
    QString new_dn = "CN=" + user_name + "," + container_dn;

    // TODO: does this work ok?
    fake_attributes[new_dn] = fake_attributes[user_dn];

    fake_object_delete(user_dn);

    fake_children[container_dn].push_back(user_dn);
}

// -----------------------------------------------------------------
// REAL STUFF
// -----------------------------------------------------------------


bool ad_interface_login() {
    if (FAKE_AD) {
        fake_ad_init();
        return true;
    }

    LDAP* ldap_connection = ad_login();
    if (ldap_connection == NULL) {
        printf("ad_login error: %s\n", ad_get_error());
        return false;
    } else {
        return true;
    }
}

// TODO: confirm that this encoding is ok
const char *qstring_to_cstr(const QString &qstr) {
    return qstr.toLatin1().constData();
}

QList<QString> load_children(const QString &dn) {
    if (FAKE_AD) {
        return fake_load_children(dn);
    }

    const char *dn_cstr = qstring_to_cstr(dn);
    char **children_raw = ad_list(dn_cstr);

    // TODO: error check

    if (children_raw != NULL) {
        auto children = QList<QString>();

        for (int i = 0; children_raw[i] != NULL; i++) {
            auto child = QString(children_raw[i]);
            children.push_back(child);
        }

        for (int i = 0; children_raw[i] != NULL; i++) {
            free(children_raw[i]);
        }
        free(children_raw);

        return children;
    } else {
        return QList<QString>();
    }
}

QMap<QString, QList<QString>> load_attributes(const QString &dn) {
    if (FAKE_AD) {
        return fake_load_attributes(dn);
    }

    // TODO: save original attributes ordering and load it like that into model

    const char *dn_cstr = qstring_to_cstr(dn);
    char** attributes_raw = ad_get_attribute(dn_cstr, NULL);

    // TODO: handle errors

    if (attributes_raw != NULL) {
        auto attributes = QMap<QString, QList<QString>>();

        // Load attributes map
        // attributes_raw is in the form of:
        // char** array of {key, value, value, key, value ...}
        // transform it into:
        // map of {key => {value, value ...}, key => {value, value ...} ...}
        for (int i = 0; attributes_raw[i + 2] != NULL; i += 2) {
            auto attribute = QString(attributes_raw[i]);
            auto value = QString(attributes_raw[i + 1]);

            // Make values list if doesn't exist yet
            if (!attributes.contains(attribute)) {
                attributes[attribute] = QList<QString>();
            }

            attributes[attribute].push_back(value);
        }

        // Free attributes_raw
        for (int i = 0; attributes_raw[i] != NULL; i++) {
            free(attributes_raw[i]);
        }
        free(attributes_raw);

        return attributes;
    } else {
        return QMap<QString, QList<QString>>();
    }
}

bool set_attribute(const QString &dn, const QString &attribute, const QString &value) {
    if (FAKE_AD) {
        fake_attributes[dn][attribute] = {value};

        return true;
    }

    const char *dn_cstr = qstring_to_cstr(dn);
    const char *attribute_cstr = qstring_to_cstr(attribute);
    const char *value_cstr = qstring_to_cstr(value);

    // TODO: handle errors

    int result = ad_mod_replace(dn_cstr, attribute_cstr, value_cstr);

    if (result == AD_SUCCESS) {
        emit ad_interface.entry_changed(dn);

        return true;
    } else {
        return false;
    }
}

// TODO: can probably make a create_anything() function with enum parameter
bool create_entry(const QString &name, const QString &dn, const QString &parent_dn, NewEntryType type) {
    // TODO: handle errors

    if (FAKE_AD) {
        switch (type) {
            case User: {
                fake_create_user(dn, parent_dn, name);
                break;
            }
            case Computer: {
                fake_create_computer(dn, parent_dn, name);
                break;
            }
            case OU: {
                fake_create_ou(dn, parent_dn, name);
                break;
            }
            case Group: {
                fake_create_group(dn, parent_dn, name);
                break;
            }
        }

        return true;
    }

    const char *name_cstr = qstring_to_cstr(name);
    const char *parent_cstr = qstring_to_cstr(parent_dn);

    int result = AD_INVALID_DN;

    switch (type) {
        case User: {
            result = ad_create_user(name_cstr, parent_cstr);
            break;
        }
        case Computer: {
            result = ad_create_computer(name_cstr, parent_cstr);
            break;
        }
        case OU: {
            result = ad_ou_create(name_cstr, parent_cstr);
            break;
        }
        case Group: {
            result = ad_group_create(name_cstr, parent_cstr);
            break;
        }
    }

    if (result == AD_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

void delete_entry(const QString &dn) {
    int result = AD_INVALID_DN;

    if (FAKE_AD) {
        fake_object_delete(dn);

        result = AD_SUCCESS;
    } else {
        const char *dn_cstr = qstring_to_cstr(dn);

        // TODO: handle all possible side-effects?
        // probably a lot of stuff, like group membership and stuff

        // TODO: handle errors

        result = ad_object_delete(dn_cstr);
    }

    if (result == AD_SUCCESS) {
        emit ad_interface.entry_deleted(dn);
    }
}

void move_user(const QString &user_dn, const QString &container_dn) {
    int result = AD_INVALID_DN;

    // TODO: duplicated
    QString user_name = extract_name_from_dn(user_dn);
    QString new_dn = "CN=" + user_name + "," + container_dn;

    printf("fake move %s %s\n", qPrintable(user_dn), qPrintable(container_dn));

    if (FAKE_AD) {
        fake_move_user(user_dn, container_dn);

        result = AD_SUCCESS;
    } else {
        const char *user_dn_cstr = qstring_to_cstr(user_dn);
        const char *container_dn_cstr = qstring_to_cstr(container_dn);

        result = ad_move_user(user_dn_cstr, container_dn_cstr);
    }

    if (result == AD_SUCCESS) {
        emit ad_interface.user_moved(user_dn, new_dn, container_dn);
    }
}

// "CN=foo,CN=bar,DC=domain,DC=com"
// =>
// "foo"
QString extract_name_from_dn(const QString &dn) {
    int equals_i = dn.indexOf('=') + 1;
    int comma_i = dn.indexOf(',');
    int segment_length = comma_i - equals_i;

    QString name = dn.mid(equals_i, segment_length);

    printf("extract_name_from_dn: %s %s\n", qPrintable(dn), qPrintable(name));

    return name;
}

// "CN=foo,CN=bar,DC=domain,DC=com"
// =>
// "CN=bar,DC=domain,DC=com"
QString extract_parent_dn_from_dn(const QString &dn) {
    int comma_i = dn.indexOf(',');

    QString parent_dn = dn.mid(comma_i + 1);

    printf("extract_parent_dn_from_dn: %s %s\n", qPrintable(dn), qPrintable(parent_dn));

    return parent_dn;
}