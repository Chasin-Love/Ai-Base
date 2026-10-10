import unreal

MAT_PATH = "/Game/Characters/M_PortalView"

def log(msg):
    unreal.log("[PortalStage7] " + msg)

def main():
    log("Creating portal view material asset...")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    mat = None
    if unreal.EditorAssetLibrary.does_asset_exist(MAT_PATH):
        log("Material already exists, updating in place.")
        mat = unreal.EditorAssetLibrary.load_asset(MAT_PATH)
    else:
        mat = asset_tools.create_asset(
            asset_name="M_PortalView",
            package_path="/Game/Characters",
            asset_class=unreal.Material,
            factory=None)
        log("create_asset returned: " + str(mat))

    if mat is None:
        unreal.log_error("[PortalStage7] Failed to create material asset at " + MAT_PATH)
        return

    # Probe confirmed: MaterialShadingModel has no MSM_UNLIT entry in this python wrapper, so
    # keep the default lit shading model and drive everything through emissive — visually
    # equivalent for a portal surface, and safer than touching a missing enum.
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    log("blend mode set to opaque")

    mat_lib = unreal.MaterialEditingLibrary

    node_tex = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, -100)
    node_tex.set_editor_property("parameter_name", "PortalView")
    node_tex.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_FROM_TEXTURE_ASSET)
    log("texture sample node created: " + str(node_tex))

    node_boost = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -350, -100)
    node_boost.set_editor_property("const_b", 2.0)
    log("multiply node created: " + str(node_boost))

    mat_lib.connect_material_expressions(node_tex, "RGB", node_boost, "A")
    mat_lib.connect_material_property(node_boost, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    log("connected emissive")

    mat_lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MAT_PATH, only_if_is_dirty=False)
    log("DONE - material saved: " + MAT_PATH)

main()
