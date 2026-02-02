#pragma once
#include<Windows.h>
#include <WinHvPlatform.h>
#define API_EXPORT extern "C" __declspec(dllexport)

//extern "C" HRESULT WINAPI WHvGetCapability(WHV_CAPABILITY_CODE CapabilityCode, VOID * CapabilityBuffer, UINT32 CapabilityBufferSizeInBytes, UINT32 * WrittenSizeInBytes);
typedef HRESULT(WINAPI* WHvGetCapability_t)(
    _In_ WHV_CAPABILITY_CODE CapabilityCode,
    _Out_writes_bytes_to_(CapabilityBufferSizeInBytes, *WrittenSizeInBytes) VOID* CapabilityBuffer,
    _In_ UINT32 CapabilityBufferSizeInBytes,
    _Out_opt_ UINT32* WrittenSizeInBytes
);
typedef HRESULT(WINAPI* WHvCreatePartition_t)(
    _Out_  WHV_PARTITION_HANDLE* Partition
);
typedef HRESULT (WINAPI* WHvSetupPartition_t)(
    _In_ WHV_PARTITION_HANDLE Partition
);

typedef HRESULT (WINAPI* WHvDeletePartition_t)(
    _In_ WHV_PARTITION_HANDLE Partition
);

typedef HRESULT (WINAPI* WHvGetPartitionProperty_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
    _Out_writes_bytes_to_(PropertyBufferSizeInBytes, *WrittenSizeInBytes) VOID* PropertyBuffer,
    _In_ UINT32 PropertyBufferSizeInBytes,
    _Out_opt_ UINT32* WrittenSizeInBytes
);

typedef HRESULT
(WINAPI* WHvSetPartitionProperty_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
    _In_reads_bytes_(PropertyBufferSizeInBytes) const VOID* PropertyBuffer,
    _In_ UINT32 PropertyBufferSizeInBytes
);

typedef HRESULT
(WINAPI* WHvMapGpaRange_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ VOID* SourceAddress,
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    _In_ UINT64 SizeInBytes,
    _In_ WHV_MAP_GPA_RANGE_FLAGS Flags
);

typedef HRESULT
(WINAPI* WHvUnmapGpaRange_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    _In_ UINT64 SizeInBytes
);

typedef HRESULT
(WINAPI* WHvTranslateGva_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,
    _In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags,
    _Out_ WHV_TRANSLATE_GVA_RESULT* TranslationResult,
    _Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa
);
typedef HRESULT
(WINAPI* WHvQueryGpaRangeDirtyBitmap_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    _In_ UINT64 RangeSizeInBytes,
    _Out_writes_bytes_opt_(BitmapSizeInBytes) UINT64* Bitmap,
    _In_ UINT32 BitmapSizeInBytes
);

typedef HRESULT(WINAPI *WHvCreateVirtualProcessor_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ UINT32 Flags
);
typedef HRESULT (WINAPI* WHvDeleteVirtualProcessor_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex
);
typedef HRESULT(WINAPI* WHvRunVirtualProcessor_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext,
    _In_ UINT32 ExitContextSizeInBytes
);

typedef HRESULT (WINAPI* WHvCancelRunVirtualProcessor_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ UINT32 Flags
);

typedef HRESULT (WINAPI*
WHvGetVirtualProcessorRegisters_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
    _In_ UINT32 RegisterCount,
    _Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
);
typedef HRESULT (WINAPI*WHvGetVirtualProcessorXsaveState_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_to_(BufferSizeInBytes, *BytesWritten) VOID* Buffer,
    _In_ UINT32 BufferSizeInBytes,
    _Out_ UINT32* BytesWritten
);
typedef HRESULT
(WINAPI*
WHvSetVirtualProcessorRegisters_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
    _In_ UINT32 RegisterCount,
    _In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues
);
typedef HRESULT
(WINAPI*
WHvSetVirtualProcessorXsaveState_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_bytes_(BufferSizeInBytes) const VOID* Buffer,
    _In_ UINT32 BufferSizeInBytes
);
typedef HRESULT
(WINAPI*
WHvGetVirtualProcessorInterruptControllerState_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State,
    _In_ UINT32 StateSize,
    _Out_opt_ UINT32* WrittenSize
);
typedef HRESULT (WINAPI*
WHvRequestInterrupt_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ const WHV_INTERRUPT_CONTROL* Interrupt,
    _In_ UINT32 InterruptControlSize
);
typedef HRESULT
(WINAPI*
    WHvSetVirtualProcessorInterruptControllerState_t)(
        _In_ WHV_PARTITION_HANDLE Partition,
        _In_ UINT32 VpIndex,
        _In_reads_bytes_(StateSize) const VOID* State,
        _In_ UINT32 StateSize
);
typedef HRESULT (WINAPI*
WHvGetPartitionCounters_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_PARTITION_COUNTER_SET CounterSet,
    _Out_writes_bytes_to_(BufferSizeInBytes, *BytesWritten) VOID* Buffer,
    _In_ UINT32 BufferSizeInBytes,
    _Out_opt_ UINT32* BytesWritten
);

typedef HRESULT
(WINAPI*
WHvGetVirtualProcessorCounters_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ WHV_PROCESSOR_COUNTER_SET CounterSet,
    _Out_writes_bytes_to_(BufferSizeInBytes, *BytesWritten) VOID* Buffer,
    _In_ UINT32 BufferSizeInBytes,
    _Out_opt_ UINT32* BytesWritten
);
typedef HRESULT(WINAPI* WHvGetVirtualProcessorInterruptControllerState2_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State,
    _In_ UINT32 StateSize,
    _Out_opt_ UINT32* WrittenSize);

typedef HRESULT(WINAPI* WHvSetVirtualProcessorInterruptControllerState2_t)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_bytes_(StateSize) const VOID* State,
    _In_ UINT32 StateSize);

typedef HRESULT(WINAPI* WHvSuspendPartitionTime_t)(
    _In_ WHV_PARTITION_HANDLE Partition);

typedef HRESULT(WINAPI* WHvResumePartitionTime_t)(_In_ WHV_PARTITION_HANDLE Partition);
void Init();
void DeInit();
void Controller();
void DeInitController();
void memscan(UINT64 address,bool isTranslation);
void ScanByteCodes();
static const WHV_REGISTER_NAME CoreRegs[] = {
    // GPRs
    WHvX64RegisterRax, WHvX64RegisterRbx, WHvX64RegisterRcx, WHvX64RegisterRdx,
    WHvX64RegisterRsi, WHvX64RegisterRdi, WHvX64RegisterRbp, WHvX64RegisterRsp,
    WHvX64RegisterR8,  WHvX64RegisterR9,  WHvX64RegisterR10, WHvX64RegisterR11,
    WHvX64RegisterR12, WHvX64RegisterR13, WHvX64RegisterR14, WHvX64RegisterR15,

    WHvX64RegisterRip,
    WHvX64RegisterRflags,

    // Segments
    WHvX64RegisterCs, WHvX64RegisterDs, WHvX64RegisterEs,
    WHvX64RegisterFs, WHvX64RegisterGs, WHvX64RegisterSs,
    WHvX64RegisterLdtr, WHvX64RegisterTr,

    // Tables
    WHvX64RegisterGdtr, WHvX64RegisterIdtr,

    // Control
    WHvX64RegisterCr0, WHvX64RegisterCr2,
    WHvX64RegisterCr3, WHvX64RegisterCr4, WHvX64RegisterCr8,

    // Debug
    WHvX64RegisterDr0, WHvX64RegisterDr1,
    WHvX64RegisterDr2, WHvX64RegisterDr3,
    WHvX64RegisterDr6, WHvX64RegisterDr7,

    WHvX64RegisterXCr0,
};
static const WHV_REGISTER_NAME FpRegs[] = {
    WHvX64RegisterXmm0,  WHvX64RegisterXmm1,
    WHvX64RegisterXmm2,  WHvX64RegisterXmm3,
    WHvX64RegisterXmm4,  WHvX64RegisterXmm5,
    WHvX64RegisterXmm6,  WHvX64RegisterXmm7,
    WHvX64RegisterXmm8,  WHvX64RegisterXmm9,
    WHvX64RegisterXmm10, WHvX64RegisterXmm11,
    WHvX64RegisterXmm12, WHvX64RegisterXmm13,
    WHvX64RegisterXmm14, WHvX64RegisterXmm15,

    WHvX64RegisterFpMmx0, WHvX64RegisterFpMmx1,
    WHvX64RegisterFpMmx2, WHvX64RegisterFpMmx3,
    WHvX64RegisterFpMmx4, WHvX64RegisterFpMmx5,
    WHvX64RegisterFpMmx6, WHvX64RegisterFpMmx7,

    WHvX64RegisterFpControlStatus,
    WHvX64RegisterXmmControlStatus,
};
static const WHV_REGISTER_NAME MsrRegs[] = {
    WHvX64RegisterTsc,
    WHvX64RegisterEfer,
    WHvX64RegisterKernelGsBase,
    WHvX64RegisterStar,
    WHvX64RegisterLstar,
    WHvX64RegisterCstar,
    WHvX64RegisterSfmask,
    WHvX64RegisterSysenterCs,
    WHvX64RegisterSysenterEip,
    WHvX64RegisterSysenterEsp,
    WHvX64RegisterApicBase,
    WHvX64RegisterPat,
};