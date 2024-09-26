#ifdef TOOLS_ENABLED

#include "godot_cpp/classes/button.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/h_slider.hpp"
#include "godot_cpp/classes/item_list.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/mesh_library.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/variant/string.hpp"
#include <godot_cpp/classes/v_box_container.hpp>

using namespace godot;

class MeshLibraryPalette : public godot::VBoxContainer {
    GDCLASS(MeshLibraryPalette, godot::VBoxContainer);

    enum DisplayMode {
        THUMBNAIL,
        LIST,
    };

private:
    Ref<MeshLibrary> mesh_library;
    LineEdit *filter_line_edit = nullptr;
    Button *mode_thumbnail_button = nullptr;
    Button *mode_list_button = nullptr;
    HSlider *icon_scale_hslider = nullptr;
    ItemList *mesh_item_list = nullptr;
    DisplayMode display_mode = DisplayMode::THUMBNAIL;
    int mesh_id = 2;

    void update_item_list();

protected:
public:
    MeshLibraryPalette();
    ~MeshLibraryPalette();

    static void _bind_methods();

    void clear_selection();
    void set_display_mode(int p_mode);
    void set_filter(String p_text);
    void set_icon_scale(float p_scale);
    void selection_changed(int p_index);
    void set_mesh(int p_mesh_id, bool update = true);
    int get_mesh();
    void set_mesh_library(Ref<MeshLibrary> p_mesh_library);
};

#endif
