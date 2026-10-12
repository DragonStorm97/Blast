// Reproductions of the defects cbe's fork fixes, one case per run, against Blast's own sources.
//
//     repro <case>      exit 0: the fixed behaviour; exit 1: the defect; a crash or a hang: the defect.
//
// See README.md beside this file for the cases, how to build, and what each run printed on `master` and on `cbe`.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include "NvBlast.h"
#include "NvBlastExtStressSolver.h"
#include "NvBlastGlobals.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
#ifdef _WIN32
    // A fault on the case's thread would otherwise sit in the system's crash handling past the two seconds, and read
    // as a hang.
    LONG WINAPI Crashed(EXCEPTION_POINTERS* info)
    {
        std::printf("CRASH (exception 0x%08lX)\n", info->ExceptionRecord->ExceptionCode);
        std::fflush(stdout);
        std::_Exit(3);
    }
#endif

    constexpr uint32_t kNone = 0xFFFFFFFFu;

    void Log(int type, const char* message, const char* file, int line)
    {
        std::printf("  blast log %d: %s (%s:%d)\n", type, message, file, line);
    }

    void* Aligned(std::vector<char>& storage, size_t size)
    {
        storage.assign(size + 16, 0);
        const uintptr_t at = (reinterpret_cast<uintptr_t>(storage.data()) + 15) & ~uintptr_t(15);
        return reinterpret_cast<void*>(at);
    }

    NvBlastChunkDesc Chunk(uint32_t parent, bool support)
    {
        NvBlastChunkDesc chunk{};
        chunk.centroid[0] = chunk.centroid[1] = chunk.centroid[2] = 0.0f;
        chunk.volume = 1.0f;
        chunk.parentChunkDescIndex = parent;
        chunk.flags = support ? NvBlastChunkDesc::SupportFlag : NvBlastChunkDesc::NoFlags;
        chunk.userData = 0;
        return chunk;
    }

    // A root and two support children, or the same three with chunk 0's parent replaced.
    std::vector<NvBlastChunkDesc> Chunks(uint32_t rootParent)
    {
        return {Chunk(rootParent, false), Chunk(0, true), Chunk(0, true)};
    }

    // A loop: chunk 0's parent is chunk 1, whose parent is chunk 0.
    std::vector<NvBlastChunkDesc> Loop()
    {
        return {Chunk(1, false), Chunk(0, true), Chunk(0, true)};
    }

    bool ReorderRefuses(std::vector<NvBlastChunkDesc> chunks)
    {
        std::vector<uint32_t> map(chunks.size());
        std::vector<char> scratch(chunks.size() * 16 + 64);
        return !NvBlastBuildAssetDescChunkReorderMap(map.data(), chunks.data(), uint32_t(chunks.size()), scratch.data(), Log);
    }

    bool CoverageRefuses(std::vector<NvBlastChunkDesc> chunks)
    {
        std::vector<char> scratch(chunks.size() * 16 + 64);
        return !NvBlastEnsureAssetExactSupportCoverage(chunks.data(), uint32_t(chunks.size()), scratch.data(), Log);
    }

    bool CreateRefuses(std::vector<NvBlastChunkDesc> chunks)
    {
        NvBlastBondDesc bond{};
        bond.chunkIndices[0] = 1;
        bond.chunkIndices[1] = 2;
        bond.bond.normal[0] = 1.0f;
        bond.bond.area = 1.0f;
        NvBlastAssetDesc desc{};
        desc.chunkCount = uint32_t(chunks.size());
        desc.chunkDescs = chunks.data();
        desc.bondCount = 1;
        desc.bondDescs = &bond;
        std::vector<char> memory, scratch;
        void* mem = Aligned(memory, NvBlastGetAssetMemorySize(&desc, Log));
        void* scr = Aligned(scratch, NvBlastGetRequiredScratchForCreateAsset(&desc, Log));
        return NvBlastCreateAsset(mem, &desc, scr, Log) == nullptr;
    }

    bool StressRefusesFamilyWithNoActor()
    {
        std::vector<NvBlastChunkDesc> chunks = Chunks(kNone);
        NvBlastBondDesc bond{};
        bond.chunkIndices[0] = 1;
        bond.chunkIndices[1] = 2;
        bond.bond.normal[0] = 1.0f;
        bond.bond.area = 1.0f;
        NvBlastAssetDesc desc{};
        desc.chunkCount = uint32_t(chunks.size());
        desc.chunkDescs = chunks.data();
        desc.bondCount = 1;
        desc.bondDescs = &bond;
        std::vector<char> assetMemory, scratch, familyMemory;
        void* mem = Aligned(assetMemory, NvBlastGetAssetMemorySize(&desc, Log));
        void* scr = Aligned(scratch, NvBlastGetRequiredScratchForCreateAsset(&desc, Log));
        const NvBlastAsset* asset = NvBlastCreateAsset(mem, &desc, scr, Log);
        if (asset == nullptr)
        {
            std::printf("  the asset was not made\n");
            return false;
        }
        // A family with no actor: NvBlastFamilyCreateFirstActor is never called.
        NvBlastFamily* family = NvBlastAssetCreateFamily(Aligned(familyMemory, NvBlastAssetGetFamilyMemorySize(asset, Log)), asset, Log);
        Nv::Blast::ExtStressSolver* solver = Nv::Blast::ExtStressSolver::create(*family);
        if (solver != nullptr)
        {
            solver->release();
            return false;
        }
        return true;
    }

    bool Run(const std::string& name, bool& known)
    {
        known = true;
        const uint32_t past = 0x40000000u;
        if (name == "reorder-loop") return ReorderRefuses(Loop());
        if (name == "coverage-loop") return CoverageRefuses(Loop());
        if (name == "reorder-out-of-range") return ReorderRefuses(Chunks(past));
        if (name == "coverage-out-of-range") return CoverageRefuses(Chunks(past));
        if (name == "create-out-of-range") return CreateRefuses(Chunks(past));
        if (name == "stress-no-actor") return StressRefusesFamilyWithNoActor();
        known = false;
        return false;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::printf("usage: repro <case>\n");
        return 2;
    }
#ifdef _WIN32
    SetUnhandledExceptionFilter(Crashed);
#endif
    const std::string name = argv[1];
    bool known = false;
    // A loop never returned: give each case two seconds.
    std::future<bool> result = std::async(std::launch::async, [&] { return Run(name, known); });
    if (result.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
    {
        std::printf("%s: HANG\n", name.c_str());
        std::fflush(stdout);
        std::_Exit(1);
    }
    const bool fixed = result.get();
    if (!known)
    {
        std::printf("unknown case %s\n", name.c_str());
        return 2;
    }
    std::printf("%s: %s\n", name.c_str(), fixed ? "refused (fixed)" : "DEFECT");
    return fixed ? 0 : 1;
}
