import unreal

MAT_PATH = "/Game/Characters/M_PortalView"

def main():
    unreal.log("[PortalStage7] Creating portal view material asset...")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    mat = None
    if unreal.EditorAssetLibrary.does_asset_exist(MAT_PATH):
        unreal.log("[PortalStage7] Material already exists, updating in place.")
        mat = unreal.EditorAssetLibrary.load_asset(MAT_PATH)
    else:
        mat = asset_tools.create_asset(
            asset_name="M_PortalView",
            package_path="/Game/Characters",
            asset_class=unreal.Material,
            factory=None)
        unreal.log("[PortalStage7] create_asset returned: " + str(mat))

    if mat is None:
        unreal.log_error("[PortalStage7] Failed to create material asset at " + MAT_PATH)
        return

    # Unlit + opaque so the portal interior reads as a bright screen
    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        unreal.log("[PortalStage7] shading_model set to unlit")
    except Exception as e:
        unreal.log_warning("[PortalStage7] shading_model set failed: " + str(e))

    unreal.log("[PortalStage7] Building node graph...")
    mat_lib = unreal.MaterialEditingLibrary

    node_tex = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, -100)
    node_tex.set_editor_property("parameter_name", "PortalView")
    node_tex.set_editor_property(" sampler_source", unreal.SamplerSourceMode.SSM_FromTextureAsset)
    unreal.log("[PortalStage7] texture sample node created: " + str(node_tex))

    node_boost = mat_lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -350, -100)
    node_boost.set_editor_property("const_b", 2.0)
    unreal.log("[PortalStage7] multiply node created: " + str(node_boost))

    mat_lib.connect_material_expressions(node_tex, "RGB", node_boost, "A")
    mat_lib.connect_material_property(node_boost, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.log("[PortalStage7] connected emissive")

    mat_lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MAT_PATH)
    unreal.log("[PortalStage7] DONE - material saved: " + MAT_PATH)

main()
