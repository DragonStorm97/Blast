# cbe's fork of NVIDIA Blast

This is NVIDIA's [Blast](https://github.com/NVIDIAGameWorks/Blast) as the CataByte Engine (cbe) builds it.

- **`master`** mirrors `NVIDIAGameWorks/Blast` and carries nothing of cbe's.
- **`cbe`** is `master` plus cbe's fixes, one commit each, and this folder. cbe pins a commit of this branch
  (`cmake/Dependencies.cmake` in cbe).

To take an upstream release, fast-forward `master` to it, merge `master` into `cbe`, run the reproductions
below, and pin the merge in cbe. Before keeping a fix, check whether upstream fixed it too.

Blast is BSD-3-Clause (`PACKAGE-LICENSES/blast-sdk-LICENSE.md`). Every file keeps its NVIDIA notice. The
files changed on `cbe` are listed below, and `git log master..cbe` shows each change.

## The fixes

Each fix is one commit on `cbe`. The numbers are the entries of cbe's `docs/notes/blast-upstream-bugs.md`.

| Commit subject | Entry | File |
|---|---|---|
| `ExtStressSolver::getExcessForces: a bond's centre is (c[0], c[1], c[2])` | 1 | `source/sdk/extensions/stress/NvBlastExtStressSolver.cpp` |
| `NvBlastExtAuthoringBuildCutoutSet: document three bytes a pixel, as it reads` | 2 | `include/extensions/authoring/NvBlastExtAuthoring.h` |
| `NvBlastExtAuthoringProcessFracture: free each chunk's triangles with delete[]` | 7 | `source/sdk/extensions/authoring/NvBlastExtAuthoring.cpp` |
| `NvBlastCreateAsset and the asset helpers: refuse a chunk parent past the end, or a loop` | 11 | `source/sdk/lowlevel/NvBlastAsset.cpp` |
| `ExtStressSolver: refuse a family with no actors, and release a refused solver safely` | 11 | `source/sdk/extensions/stress/NvBlastExtStressSolver.cpp` |
| `BooleanToolImpl::performBoolean: size mesh B's default accelerator by mesh B` | 13 | `source/sdk/extensions/authoring/NvBlastExtAuthoringBooleanToolImpl.cpp` |

How each fix is tested:

- **Entry 1, in cbe.** cbe's native test "a piece stress breaks off is turned by the torque its broken bonds bore
  about its centre" (`tests/Destruction/TestDestructionStress.cpp`) fails on `master` and passes on `cbe`.
- **Entry 11, here.** The reproductions in `repro/` cover both commits.
- **Entries 2, 7 and 13 are not executed here.** Entry 2 changes only documentation. Entry 7 changes `delete` to
  `delete[]`. Entry 13 changes `meshA` to `meshB`. cbe passes its own accelerators to the boolean tool, so cbe
  never reaches the default accelerators that entry 13 fixes.

## The reproductions

`repro/` builds one executable from this checkout's own sources: the low-level library, the globals and the
stress solver. It runs each case in its own process, because an unfixed case can crash or hang:

```
pwsh cbe/repro/run.ps1
```

Run it from a Visual Studio developer shell, or anywhere CMake finds a C++ compiler. It exits non-zero
when any case shows its defect. Results with MSVC, x64, Release:

| Case | `master` (636e7eb) | `cbe` |
|---|---|---|
| `reorder-loop`: `NvBlastBuildAssetDescChunkReorderMap`, two chunks each other's parent | hangs | refused |
| `coverage-loop`: `NvBlastEnsureAssetExactSupportCoverage`, the same loop | hangs | refused |
| `reorder-out-of-range`: the reorder map, a parent index of 0x40000000 | access violation | refused |
| `coverage-out-of-range`: exact coverage, the same index | access violation | refused |
| `create-out-of-range`: `NvBlastCreateAsset`, the same index | access violation | refused |
| `stress-no-actor`: `ExtStressSolver::create` for a family whose first actor was never made | access violation | `nullptr`, logged |
