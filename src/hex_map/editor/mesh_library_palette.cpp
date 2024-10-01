#ifdef TOOLS_ENABLED

#include "mesh_library_palette.h"
#include "godot_cpp/classes/button.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_plugin.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/theme.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/vector2i.hpp"

MeshLibraryPalette::MeshLibraryPalette() {
    Ref<Theme> theme = EditorInterface::get_singleton()->get_editor_theme();

    HBoxContainer *hbox = memnew(HBoxContainer);
    hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    hbox->set_custom_minimum_size(Vector2(230, 0));
    add_child(hbox);

    filter_line_edit = memnew(LineEdit);
    filter_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    filter_line_edit->set_placeholder("Filter Meshes");
    filter_line_edit->set_clear_button_enabled(true);
    filter_line_edit->set_right_icon(theme->get_icon("Search", "EditorIcons"));
    filter_line_edit->connect("text_changed",
            callable_mp(this, &MeshLibraryPalette::set_filter));
    hbox->add_child(filter_line_edit);
    // XXX gui input up/down pageup/pagedown sent keycode to itemlist

    mode_list_button = memnew(Button);
    mode_list_button->set_theme_type_variation("FlatButton");
    mode_list_button->set_toggle_mode(true);
    mode_list_button->set_pressed(false);
    mode_list_button->set_button_icon(
            theme->get_icon("FileList", "EditorIcons"));
    mode_list_button->connect("pressed",
            callable_mp(this, &MeshLibraryPalette::set_display_mode)
                    .bind(DisplayMode::LIST));
    hbox->add_child(mode_list_button);

    mode_thumbnail_button = memnew(Button);
    mode_thumbnail_button->set_theme_type_variation("FlatButton");
    mode_thumbnail_button->set_toggle_mode(true);
    mode_thumbnail_button->set_pressed(true);
    mode_thumbnail_button->set_button_icon(
            theme->get_icon("FileThumbnail", "EditorIcons"));
    mode_thumbnail_button->connect("pressed",
            callable_mp(this, &MeshLibraryPalette::set_display_mode)
                    .bind(DisplayMode::THUMBNAIL));
    hbox->add_child(mode_thumbnail_button);

    icon_scale_hslider = memnew(HSlider);
    icon_scale_hslider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    icon_scale_hslider->set_min(0.2f);
    icon_scale_hslider->set_max(4.0f);
    icon_scale_hslider->set_step(0.1f);
    icon_scale_hslider->set_value(1.0f);
    icon_scale_hslider->connect("value_changed",
            callable_mp(this, &MeshLibraryPalette::set_icon_scale));
    add_child(icon_scale_hslider);

    mesh_item_list = memnew(ItemList);
    mesh_item_list->set_auto_translate(false);
    mesh_item_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    mesh_item_list->connect("item_selected",
            callable_mp(this, &MeshLibraryPalette::selection_changed));
    add_child(mesh_item_list);
    // XXX gui input cmd/ctrl scroll change icon scale
    // _mesh_library_palette_input
}

MeshLibraryPalette::~MeshLibraryPalette() {}

void MeshLibraryPalette::_bind_methods() {
    // not in love with using a signal here, but using a method to update
    // mesh_library
    ADD_SIGNAL(
            MethodInfo("mesh_changed", PropertyInfo(Variant::INT, "mesh_id")));
}

void MeshLibraryPalette::set_display_mode(int p_mode) {
    switch ((DisplayMode)p_mode) {
        case DisplayMode::THUMBNAIL:
            mode_list_button->set_pressed(false);
            mode_thumbnail_button->set_pressed(true);
            break;
        case DisplayMode::LIST:
            mode_thumbnail_button->set_pressed(false);
            mode_list_button->set_pressed(true);
            break;
        default:
            ERR_FAIL_EDMSG("invalid display mode");
    }
    display_mode = (DisplayMode)p_mode;
    update_item_list();
}

void MeshLibraryPalette::clear_selection() {
    mesh_item_list->deselect_all();
    set_mesh(-1, false);
}

int MeshLibraryPalette::get_mesh() {
    if (!mesh_item_list->is_anything_selected()) {
        return -1;
    }
    int index = mesh_item_list->get_selected_items()[0];
    return mesh_item_list->get_item_metadata(index);
}

void MeshLibraryPalette::set_filter(String filter) {
    static String last_value = "";
    if (last_value != filter) {
        last_value = filter;
        update_item_list();
    }
}

void MeshLibraryPalette::set_icon_scale(float scale) {
    mesh_item_list->set_icon_scale(scale);
}

void MeshLibraryPalette::selection_changed(int index) {
    UtilityFunctions::print("set_selection() ", index);
    set_mesh(mesh_item_list->get_item_metadata(index), false);
}

void MeshLibraryPalette::set_mesh(int p_mesh_id, bool update) {
    // save off the new value
    this->mesh_id = p_mesh_id;

    // if the caller wants to update the ItemList, do it now
    if (update) {
        int count = mesh_item_list->get_item_count();
        bool found = false;
        for (int i = 0; i < count; i++) {
            if ((int)mesh_item_list->get_item_metadata(i) == p_mesh_id) {
                mesh_item_list->select(i);
                found = true;
                break;
            }
        }

        // if the selected mesh isn't in the item list, clear the filter and
        // rebuild the item list; the correct item will be selected during the
        // rebuild
        if (!found) {
            filter_line_edit->clear();
            update_item_list();
        }
    }

    UtilityFunctions::print("set_mesh() ", mesh_id);
    emit_signal("mesh_changed", mesh_id);
}

void MeshLibraryPalette::set_mesh_library(Ref<MeshLibrary> p_mesh_library) {
    mesh_library = p_mesh_library;
    update_item_list();
}

void MeshLibraryPalette::update_item_list() {
    float min_size = 128; // XXX scale to UI?

    // clear the selected id
    mesh_id = -1;
    mesh_item_list->clear();
    switch (display_mode) {
        case DisplayMode::THUMBNAIL:
            mesh_item_list->set_max_columns(0);
            mesh_item_list->set_icon_mode(ItemList::ICON_MODE_TOP);
            mesh_item_list->set_fixed_column_width(
                    min_size * MAX(icon_scale_hslider->get_value(), 1.5));
            break;
        case DisplayMode::LIST:
            mesh_item_list->set_max_columns(1);
            mesh_item_list->set_icon_mode(ItemList::ICON_MODE_LEFT);
            mesh_item_list->set_fixed_column_width(0);
            break;
        default:
            ERR_FAIL_EDMSG("invalid display mode");
    }
    mesh_item_list->set_fixed_icon_size(Vector2i(min_size, min_size));
    mesh_item_list->set_max_text_lines(2);

    if (mesh_library.is_null()) {
        filter_line_edit->clear();
        filter_line_edit->set_editable(false);
        // XXX show note to set meshlibrary
        return;
    }

    filter_line_edit->set_editable(true);
    // XXX hide note

    String filter = filter_line_edit->get_text().strip_edges();
    PackedInt32Array ids = mesh_library->get_item_list();
    for (const int id : ids) {
        String name = mesh_library->get_item_name(id);
        if (name.is_empty()) {
            name = "#" + itos(id);
        }

        if (!filter.is_empty() && !filter.is_subsequence_ofn(name)) {
            continue;
        }

        Ref<Texture2D> icon = mesh_library->get_item_preview(id);
        int index = mesh_item_list->add_item(name, icon);
        mesh_item_list->set_item_tooltip(index, name);
        mesh_item_list->set_item_metadata(index, id);
        if (this->mesh_id == id) {
            mesh_item_list->select(index);
        }
    }
    mesh_item_list->sort_items_by_text();
    mesh_item_list->set_icon_scale(icon_scale_hslider->get_value());
}
#endif
