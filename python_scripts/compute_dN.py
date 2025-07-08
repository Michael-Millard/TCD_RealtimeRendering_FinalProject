import bpy
import bmesh
import mathutils
from mathutils import Vector, bvhtree

# === Settings ===
max_distance = 1000.0  # Maximum ray length

# === Step 1: Get the active mesh object ===
obj = bpy.context.object
if obj is None or obj.type != 'MESH':
    raise Exception("Please select a mesh object.")

# Apply transforms (optional but recommended)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

mesh = obj.data

# === Step 2: Create a BMesh and BVHTree from the object ===
bm = bmesh.new()
bm.from_mesh(mesh)
bm.verts.ensure_lookup_table()
bm.faces.ensure_lookup_table()
bvh = bvhtree.BVHTree.FromBMesh(bm)

# === Step 3: Create or reuse a float vertex attribute for d_N ===
if "d_N" in mesh.attributes:
    mesh.attributes.remove(mesh.attributes["d_N"])
d_n_attr = mesh.attributes.new(name="d_N", type='FLOAT', domain='POINT')

# === Step 4: For each vertex, cast a ray in the -normal direction ===
print("Computing d_N for each vertex...")

for i, vert in enumerate(bm.verts):
    origin = vert.co
    normal = vert.normal.normalized()
    direction = -normal

    # Get all intersections
    hits = bvh.ray_cast_all(origin, direction, max_distance)
    if not hits:
        d_n_attr.data[i].value = 0.0  # No hit found
        continue

    # Get furthest hit point
    max_dist = 0.0
    for hit in hits:
        location, normal_hit, index, distance = hit
        if distance > max_dist:
            max_dist = distance

    d_n_attr.data[i].value = max_dist

print("Finished computing d_N values.")

# === Step 5: Save as FBX ===
output_path = bpy.path.abspath("//with_dN.fbx")
bpy.ops.export_scene.fbx(
    filepath=output_path,
    use_selection=True,
    apply_unit_scale=True,
    bake_space_transform=True,
    add_leaf_bones=False,
    path_mode='AUTO',
    embed_textures=False
)

print(f"Exported to {output_path}")
