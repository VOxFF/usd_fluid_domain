# usd_fluid_domain

C++ library for CFD pre-processing using OpenUSD. Given an input USD scene, it
builds a fluid simulation domain and a watertight envelope surface, then
composes everything into a single root USD layer ready for inspection or
downstream processing.

## Dependencies

| Library | Version |
|---------|---------|
| OpenUSD | 21.x+ |
| OpenVDB | 8.x |

## Typical workflow

```cpp
// 1. Load input geometry
ufd::StageReader reader;
reader.open("scene.usda");
auto meshes = reader.collect_meshes();

// 2. Compute bounding box
ufd::SurfaceExtractor extractor;
auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

// 3. Build fluid domain
auto domain_stage = pxr::UsdStage::CreateNew("domain.usda");
ufd::DomainBuilder(ufd::DomainConfig{}).build(domain_stage, bounds);

// 4. Build watertight envelope
auto envelope_stage = pxr::UsdStage::CreateNew("envelope.usda");
ufd::EnvelopeBuilder(ufd::EnvelopeConfig{}).build(envelope_stage, meshes);

// 5. Compose into a root layer
ufd::StageComposer composer("root.usda");
composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
composer.write();
```

## Classes

### `StageReader`

Opens a USD file and traverses it to collect geometry prims.

```cpp
bool open(const std::string& path);
std::vector<UsdGeomMesh> collect_meshes() const;
UsdStageRefPtr get_stage() const;
```

### `SurfaceExtractor`

Merges mesh point data and computes axis-aligned bounding boxes. Used to
determine the extents passed to `DomainBuilder`.

```cpp
SurfaceData extract(const std::vector<UsdGeomMesh>& meshes) const;
GfRange3d   compute_bounding_box(const SurfaceData& surface) const;
```

### `DomainConfig`

Configuration for the fluid simulation domain.

| Field | Default | Description |
|-------|---------|-------------|
| `shape` | `Box` | Domain shape: `Box` or `Cylinder` |
| `extent_multiplier` | `10.0` | Scale factor applied to input bounding box |
| `flow_direction` | `(1,0,0)` | Primary flow axis (used as cylinder axis) |
| `origin_offset` | `(0,0,0)` | Manual offset from object centroid |
| `cylinder_segments` | `36` | Polygon count around the cylinder |
| `symmetry_y` | `false` | Generate half-domain (XZ symmetry plane) |

### `DomainBuilder`

Generates a `UsdGeomMesh` at `/FluidDomain` on the provided stage. The mesh
represents the far-field boundary of the simulation (box or cylinder shaped),
sized relative to the input geometry bounding box.

```cpp
DomainBuilder(const DomainConfig& config);
std::string build(UsdStageRefPtr stage, const GfRange3d& object_bounds) const;
// returns "/FluidDomain"
```

### `EnvelopeConfig`

Configuration for the watertight envelope surface.

| Field | Default | Description |
|-------|---------|-------------|
| `voxel_size` | `0.1` | VDB voxel edge length in world units |
| `hole_threshold` | `0.5` | Morphological closing radius; bridges holes smaller than this |

### `EnvelopeBuilder`

Builds a watertight outer surface from the input meshes using an OpenVDB
signed-distance-field pipeline: meshes are unioned, morphological closing
bridges small holes and gaps, and the zero level set is iso-surfaced back into
a polygon mesh written at `/Envelope`.

```cpp
EnvelopeBuilder(const EnvelopeConfig& config = {});
std::string build(UsdStageRefPtr stage,
                  const std::vector<UsdGeomMesh>& meshes) const;
// returns "/Envelope", or "" if meshes is empty
```

### `StageComposer`

Assembles component stages into a composed root USD layer. Components are
ordered by `ComponentType` — higher enum value wins conflicts in the sublayer
stack. Materials (UsdPreviewSurface) are automatically authored on each
non-input component stage before saving.

```cpp
StageComposer(const std::string& root_path);
void add_component(ComponentType type, UsdStageRefPtr stage);
bool write() const;
```

**Component types and sublayer order (strongest → weakest):**

| `ComponentType` | Prim | Material color |
|-----------------|------|----------------|
| `Envelope` | `/Envelope` | green (0.2, 0.8, 0.2), opacity 0.75 |
| `FluidDomain` | `/FluidDomain` | blue (0.2, 0.5, 0.8), opacity 0.30 |
| `InputGeometry` | *(unchanged)* | *(none applied)* |

## CLI

The `usd_fluid_domain` executable runs the full pipeline on a USD file:

```sh
usd_fluid_domain <input.usd> <output.usd>
```

Three files are written:

| File | Contents |
|------|----------|
| `<output.usd>.domain.usda` | Fluid domain mesh (`/FluidDomain`) |
| `<output.usd>.envelope.usda` | Watertight envelope mesh (`/Envelope`) |
| `<output.usd>` | Root layer compositing all three components |

Open the root layer in usdview to inspect all components together:

```sh
usdview output.usd
```

## Build

```sh
cmake -B build -DUSD_ROOT=/path/to/usd
cmake --build build
ctest --test-dir build
```
