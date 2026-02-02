#include "pch.h"
#include "exports.h"
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <mutex>
#include <chrono>
#include "linuxDefs.h"
WHvGetCapability_t _WHvGetCapability = nullptr;
WHvCreatePartition_t _WHvCreatePartition = nullptr;
WHvSetupPartition_t _WHvSetupPartition = nullptr;
WHvDeletePartition_t _WHvDeletePartition = nullptr;
WHvGetPartitionProperty_t _WHvGetPartitionProperty = nullptr;
WHvSetPartitionProperty_t _WHvSetPartitionProperty = nullptr;
WHvMapGpaRange_t _WHvMapGpaRange = nullptr;
WHvUnmapGpaRange_t _WHvUnmapGpaRange = nullptr;
WHvTranslateGva_t _WHvTranslateGva = nullptr;
WHvQueryGpaRangeDirtyBitmap_t _WHvQueryGpaRangeDirtyBitmap = nullptr;
WHvCreateVirtualProcessor_t _WHvCreateVirtualProcessor = nullptr;
WHvDeleteVirtualProcessor_t _WHvDeleteVirtualProcessor = nullptr;
WHvRunVirtualProcessor_t _WHvRunVirtualProcessor = nullptr;
WHvCancelRunVirtualProcessor_t _WHvCancelRunVirtualProcessor = nullptr;
WHvGetVirtualProcessorRegisters_t _WHvGetVirtualProcessorRegisters = nullptr;
WHvGetVirtualProcessorXsaveState_t _WHvGetVirtualProcessorXsaveState = nullptr;
WHvSetVirtualProcessorRegisters_t _WHvSetVirtualProcessorRegisters = nullptr;
WHvSetVirtualProcessorXsaveState_t _WHvSetVirtualProcessorXsaveState = nullptr;
WHvGetVirtualProcessorInterruptControllerState_t _WHvGetVirtualProcessorInterruptControllerState = nullptr;
WHvRequestInterrupt_t _WHvRequestInterrupt = nullptr;
WHvGetPartitionCounters_t _WHvGetPartitionCounters = nullptr;
WHvSetVirtualProcessorInterruptControllerState_t _WHvSetVirtualProcessorInterruptControllerState = nullptr;
WHvGetVirtualProcessorCounters_t _WHvGetVirtualProcessorCounters = nullptr;
WHvGetVirtualProcessorInterruptControllerState2_t _WHvGetVirtualProcessorInterruptControllerState2 = nullptr;
WHvSetVirtualProcessorInterruptControllerState2_t _WHvSetVirtualProcessorInterruptControllerState2 = nullptr;
WHvSuspendPartitionTime_t _WHvSuspendPartitionTime = nullptr;
WHvResumePartitionTime_t _WHvResumePartitionTime = nullptr;

HMODULE lib;


WHV_PARTITION_HANDLE HookPartition;
std::vector<UINT32> VPindexs;
void* ramLocInHost = nullptr;
UINT64 ramSize = -1;
const UINT32 regIndex = 4;
bool CreateVP = false;
struct GpaMemoryArea {
	WHV_GUEST_PHYSICAL_ADDRESS gpa_start; // page-aligned
	WHV_GUEST_PHYSICAL_ADDRESS gpa_end;   // exclusive
	UINT64 size;                          // page-aligned size
	UINT8* host_ptr = nullptr;           // backing host memory
	WHV_MAP_GPA_RANGE_FLAGS orignal_perms;
	WHV_GUEST_PHYSICAL_ADDRESS actual_addr;
};
UINT64 GivenAddress;
UINT64 resultAddress = 0;
UINT64 linker64 = 0;
UINT64 linker64Base = 0;
bool TranslateOnly = false;
std::mutex hookCriticalMutex;

std::vector<void*> foundptr;
std::vector<GpaMemoryArea*> hookRegions;

short counter = 0;

void Logs(std::string log) {
	std::ofstream f("logs.txt", std::ios_base::app);
	f << log << std::endl;
	f.close();
}
std::string to_hex(uint64_t value) {
	std::stringstream ss;
	ss << "0x" << std::hex << std::uppercase << value;
	return ss.str();
}
void bmh_search_all(const UINT8* haystack, size_t hlen,const UINT8* needle, size_t nlen)
{
	foundptr.clear();

	if (nlen == 0 || nlen > hlen)
		return;

	size_t shift[256];
	for (size_t i = 0; i < 256; i++)
		shift[i] = nlen;

	for (size_t i = 0; i < nlen - 1; i++)
		shift[needle[i]] = nlen - 1 - i;

	size_t pos = 0;

	while (pos <= hlen - nlen) {
		size_t j = nlen - 1;

		while (haystack[pos + j] == needle[j]) {
			if (j == 0) {
				foundptr.push_back((void*)(haystack + pos));
				pos += 1;
				goto next_iteration;
			}
			j--;
		}

		pos += shift[haystack[pos + nlen - 1]];

	next_iteration:
		;
	}
}
bool ChangeMemoryPerms(WHV_GUEST_PHYSICAL_ADDRESS gpa,UINT64 size,GpaMemoryArea& outArea) {
	if (!ramLocInHost) {
		Logs("Invalid Ram");
		return false;
	}

	const UINT64 PAGE_SIZE = 0x1000;
	WHV_GUEST_PHYSICAL_ADDRESS page_gpa = gpa & ~(PAGE_SIZE - 1);
	UINT64 map_size =
		((gpa + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)) - page_gpa;
	UINT8* host_ptr = (UINT8*)ramLocInHost + page_gpa;

	outArea.gpa_start = page_gpa;
	outArea.gpa_end = page_gpa + map_size;
	outArea.size = map_size;
	outArea.host_ptr = host_ptr;
	outArea.actual_addr = gpa;
	WHvUnmapGpaRange(HookPartition, page_gpa, map_size);

	HRESULT hr = WHvMapGpaRange(
		HookPartition,
		host_ptr,
		page_gpa,
		map_size,
		WHvMapGpaRangeFlagRead|WHvMapGpaRangeFlagWrite
	);

	if (FAILED(hr)) {
		Logs("Error in Mapping GPA");
		return false;
	}
	//Logs("EXEC removed | GPA [0x" +std::to_string(outArea.gpa_start) +" - 0x" +std::to_string(outArea.gpa_end) +") size=0x" +std::to_string(outArea.size));
	return true;
}
bool ChangeMemoryPerms(GpaMemoryArea& inArea) {
	if (!ramLocInHost) {
		Logs("Invalid Ram");
		return false;
	}
	WHvUnmapGpaRange(HookPartition, inArea.gpa_start, inArea.size);

	HRESULT hr = WHvMapGpaRange(
		HookPartition,
		inArea.host_ptr,
		inArea.gpa_start,
		inArea.size,
		WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite
	);

	if (FAILED(hr)) {
		Logs("Error in Mapping GPA");
		return false;
	}

	//Logs("EXEC removed | GPA [0x" + std::to_string(inArea.gpa_start) + " - 0x" + std::to_string(inArea.gpa_end) + ") size=0x" + std::to_string(inArea.size));
	return true;
}
bool RevertMemoryPerms(GpaMemoryArea& inArea) {
	if (!ramLocInHost) {
		Logs("Invalid Ram");
		return false;
	}
	WHvUnmapGpaRange(HookPartition, inArea.gpa_start, inArea.size);
	HRESULT hr = WHvMapGpaRange(HookPartition,inArea.host_ptr, inArea.gpa_start, inArea.size,WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite| WHvMapGpaRangeFlagExecute);
	if (FAILED(hr)) {
		Logs("Error in Reverting GPA");
		return false;
	}
	return true;
}
void GetCPUContext(UINT64 VPIndex) {
	const WHV_REGISTER_NAME allRegister[] = {
	WHvX64RegisterRax,
	WHvX64RegisterRcx,
	WHvX64RegisterRdx,
	WHvX64RegisterRbx,
	WHvX64RegisterRsp,
	WHvX64RegisterRbp,
	WHvX64RegisterRsi,
	WHvX64RegisterRdi,
	WHvX64RegisterR8,
	WHvX64RegisterR9,
	WHvX64RegisterR10,
	WHvX64RegisterR11,
	WHvX64RegisterR12,
	WHvX64RegisterR13,
	WHvX64RegisterR14,
	WHvX64RegisterR15,
	WHvX64RegisterRip,
	WHvX64RegisterRflags,
	};
	WHV_REGISTER_VALUE allVals[18];
	_WHvGetVirtualProcessorRegisters(HookPartition, VPIndex,allRegister,18 ,allVals);
	Logs("Rax " + to_hex(allVals[0].Reg64));
	Logs("Rcx " + to_hex(allVals[1].Reg64));
	Logs("Rdx " + to_hex(allVals[2].Reg64));
	Logs("Rbx " + to_hex(allVals[3].Reg64));
	Logs("Rsp " + to_hex(allVals[4].Reg64));
	Logs("Rbp " + to_hex(allVals[5].Reg64));
	Logs("Rsi " + to_hex(allVals[6].Reg64));
	Logs("Rdi " + to_hex(allVals[7].Reg64));
	Logs("r8 " + to_hex(allVals[8].Reg64));
	Logs("r9 " + to_hex(allVals[9].Reg64));
	Logs("r10 " + to_hex(allVals[10].Reg64));
	Logs("r11 " + to_hex(allVals[11].Reg64));
	Logs("r12 " + to_hex(allVals[12].Reg64));
	Logs("r13 " + to_hex(allVals[13].Reg64));
	Logs("r14 " + to_hex(allVals[14].Reg64));
	Logs("r15 " + to_hex(allVals[15].Reg64));
	Logs("RIP " + to_hex(allVals[16].Reg64));
	Logs("RFlags " + to_hex(allVals[17].Reg64));
}
bool TransferCPUCtx(UINT64 fromIndex,UINT64 toIndex) {
	auto copy_group =
		[&](const WHV_REGISTER_NAME* regs, UINT32 count) -> bool {

		std::vector<WHV_REGISTER_VALUE> values(count);

		HRESULT hr = WHvGetVirtualProcessorRegisters(
			HookPartition, fromIndex, regs, count, values.data()
		);
		if (FAILED(hr)) return false;

		hr = WHvSetVirtualProcessorRegisters(
			HookPartition, toIndex, regs, count, values.data()
		);
		return SUCCEEDED(hr);
	};

	if (!copy_group(CoreRegs, ARRAYSIZE(CoreRegs)))
		return false;

	if (!copy_group(FpRegs, ARRAYSIZE(FpRegs)))
		return false;

	if (!copy_group(MsrRegs, ARRAYSIZE(MsrRegs)))
		return false;

	return true;
}
void _EnableSingleStep(UINT64 vcpuIndex) {
	WHV_REGISTER_NAME name = WHvX64RegisterRflags;
	WHV_REGISTER_VALUE value;

	_WHvGetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);
	value.Reg64 |= 0x100;
	HRESULT hr = _WHvSetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);
	name = WHvRegisterInterruptState;
	value.Reg64 = 0;
	value.InterruptState.InterruptShadow = false;
	hr = _WHvSetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);
}
void _DisableSingleStep(UINT64 vcpuIndex) {
	HRESULT hr;
	WHV_REGISTER_NAME name = WHvX64RegisterRflags;
	WHV_REGISTER_VALUE value;
	_WHvGetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);

	value.Reg64 &= ~0x100;

	_WHvSetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);
	name = WHvRegisterInterruptState;
	value.Reg64 = 0;
	value.InterruptState.InterruptShadow = true;
	hr = _WHvSetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);

	_WHvGetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);
	if (value.PendingDebugException.SingleStep) {
		value.PendingDebugException.SingleStep = 0;
		hr = _WHvSetVirtualProcessorRegisters(HookPartition, vcpuIndex, &name, 1, &value);
	}
}
unsigned char dl_open[] = { 0x55, 0x48, 0x89, 0xe5, 0x48, 0x83, 0xec, 0x20, 0x48, 0x89, 0x7d, 0xf8, 0x89, 0x75, 0xf4, 0xc7, 0x45, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0xc7, 0x45, 0xec, 0x5a, 0x00, 0x00, 0x00, 0xc7, 0x45, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x45, 0xe8, 0x3b, 0x45, 0xf4, 0x0f, 0x8d, 0x20, 0x00, 0x00, 0x00, 0x8b, 0x45, 0xf0, 0x83, 0xc0, 0xff, 0x89, 0x45, 0xf0, 0x8b, 0x45, 0xf0, 0x83, 0xc0, 0x5a, 0x89, 0x45, 0xec, 0x8b, 0x45, 0xe8, 0x83, 0xc0, 0x01, 0x89, 0x45, 0xe8, 0xe9, 0xd4, 0xff, 0xff, 0xff, 0x48, 0x8d, 0x3d, 0x4f, 0x21, 0xff, 0xff, 0x31, 0xf6, 0xe8, 0x42, 0xfc, 0x02, 0x00, 0x83, 0xf8, 0x00, 0x0f, 0x85, 0x17, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x45, 0xf8, 0x8a, 0x08, 0x80, 0xf1, 0xff, 0x48, 0x8b, 0x45, 0xf8, 0x80, 0xe1, 0x01, 0x88, 0x08, 0xe9, 0x05, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc4, 0x20, 0x5d, 0xc3 };
void memscan(UINT64 address,bool isTranslation) {
	GivenAddress = address;
	resultAddress = 0;
	TranslateOnly = isTranslation;
}
void ScanByteCodes() {
	bmh_search_all((uint8_t*)ramLocInHost, ramSize,(uint8_t*)dl_open, sizeof(dl_open));
	for (int i = 0; i < foundptr.size();i++) {
		char test[100] = { 0 };
		memcpy(test, foundptr[i], 100);
		Logs("found ptr");
		GpaMemoryArea* area = new GpaMemoryArea;
		UINT phys = (((UINT64)foundptr[i])-(UINT64)ramLocInHost);
		ChangeMemoryPerms(phys, 0x100, *area);
		hookRegions.push_back(area);
	}

}
//void CustomLogic(UINT32 VpIndex) {
//	WHV_REGISTER_NAME rname = WHV_REGISTER_NAME::WHvX64RegisterRdi;
//	WHV_REGISTER_VALUE rdi;
//	_WHvGetVirtualProcessorRegisters(HookPartition, VpIndex, &rname, 1, &rdi);
//	rdi.Reg64 = 0xf;
//	_WHvSetVirtualProcessorRegisters(HookPartition, VpIndex, &rname, 1, &rdi);
//	//WHV_TRANSLATE_GVA_RESULT rdiRes;
//	//WHV_GUEST_PHYSICAL_ADDRESS commAddr;
//	//_WHvTranslateGva(HookPartition, VpIndex, rdi.Reg64, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateRead, &rdiRes, &commAddr);
//	//if (rdiRes.ResultCode == WHvTranslateGvaResultSuccess) {
//		//Logs(std::string((char*)ramLocInHost + commAddr));
//		/*auto f = reinterpret_cast<PATH_RAW*>((char*)ramLocInHost + commAddr);
//		_WHvTranslateGva(HookPartition, VpIndex, (UINT64)f->dentry, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateRead, &rdiRes, &commAddr);
//		auto D = reinterpret_cast<DENTRY_RAW*>((char*)ramLocInHost + commAddr);
//		_WHvTranslateGva(HookPartition, VpIndex, (UINT64)D->d_name.name, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateRead, &rdiRes, &commAddr);
//		std::string comm = std::string((char*)ramLocInHost + commAddr);
//		Logs(comm);*/
//	//}
//}
void CustomLogic(UINT32 VpIndex) {
	WHV_REGISTER_NAME rname = WHV_REGISTER_NAME::WHvX64RegisterRdi;
	WHV_REGISTER_VALUE rdi;
	_WHvGetVirtualProcessorRegisters(HookPartition, VpIndex, &rname, 1, &rdi);
	WHV_TRANSLATE_GVA_RESULT rdiRes;
	WHV_GUEST_PHYSICAL_ADDRESS commAddr;
	_WHvTranslateGva(HookPartition, VpIndex, rdi.Reg64, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateRead, &rdiRes, &commAddr);
	if (rdiRes.ResultCode == WHvTranslateGvaResultSuccess) {
		bool* varLoc = (bool*)((char*)ramLocInHost + commAddr);
		*varLoc = 0x1;
		//Logs("["+std::to_string(duration.count())+"ns ] " + x);
		/*auto f = reinterpret_cast<PATH_RAW*>((char*)ramLocInHost + commAddr);
		_WHvTranslateGva(HookPartition, VpIndex, (UINT64)f->dentry, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateRead, &rdiRes, &commAddr);
		auto D = reinterpret_cast<DENTRY_RAW*>((char*)ramLocInHost + commAddr);
		_WHvTranslateGva(HookPartition, VpIndex, (UINT64)D->d_name.name, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateRead, &rdiRes, &commAddr);
		std::string comm = std::string((char*)ramLocInHost + commAddr);
		Logs(comm);*/
	}	
}
HRESULT WINAPI WHvGetCapability(_In_ WHV_CAPABILITY_CODE CapabilityCode,_Out_writes_bytes_to_(CapabilityBufferSizeInBytes, *WrittenSizeInBytes) VOID* CapabilityBuffer,_In_ UINT32 CapabilityBufferSizeInBytes,_Out_opt_ UINT32* WrittenSizeInBytes) {
	//Logs("WHvGetCapability Called");
	HRESULT hr = _WHvGetCapability(CapabilityCode, CapabilityBuffer, CapabilityBufferSizeInBytes, WrittenSizeInBytes);
	return hr;
}
HRESULT WINAPI WHvCreatePartition(_Out_ WHV_PARTITION_HANDLE * Partition) {
	Logs("WHvCreatePartition Called");
	HRESULT hr = _WHvCreatePartition(Partition);
	HookPartition = *Partition;
	return hr;
}
HRESULT WINAPI WHvSetupPartition(_In_ WHV_PARTITION_HANDLE Partition) {
	WHV_PARTITION_PROPERTY props;
	UINT32 garbage;
	_WHvGetPartitionProperty(HookPartition, WHvPartitionPropertyCodeProcessorCount,&props,sizeof(props),&garbage);
	//Logs("WHvSetupPartition Called " + std::to_string(props.ProcessorCount));
	_WHvGetPartitionProperty(HookPartition, WHvPartitionPropertyCodeExceptionExitBitmap, &props, sizeof(props), &garbage);
	uint64_t bitmap = props.ExceptionExitBitmap;
	bitmap |= (1ull << WHvX64ExceptionTypeDebugTrapOrFault);
	props.ExceptionExitBitmap = bitmap;
	_WHvSetPartitionProperty(HookPartition, WHvPartitionPropertyCodeExceptionExitBitmap, &props, sizeof(props));
	HRESULT hr = _WHvSetupPartition(Partition);
	_WHvGetPartitionProperty(HookPartition, WHvPartitionPropertyCodeExtendedVmExits, &props, sizeof(props), &garbage);
	props.ExtendedVmExits.ExceptionExit = 1;
	_WHvSetPartitionProperty(HookPartition, WHvPartitionPropertyCodeExtendedVmExits, &props, sizeof(props));
	hr = _WHvCreateVirtualProcessor(HookPartition, 4, 0);
	if (hr != S_OK) {
		Logs("Virtual Processor Fail");
	}
	return hr;
}
HRESULT WINAPI WHvDeletePartition(_In_ WHV_PARTITION_HANDLE Partition) {
	Logs("WHvDeletePartition Called");
	return _WHvDeletePartition(Partition);
}
HRESULT WINAPI WHvGetPartitionProperty(_In_ WHV_PARTITION_HANDLE Partition,_In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,_Out_writes_bytes_to_(PropertyBufferSizeInBytes, *WrittenSizeInBytes) VOID * PropertyBuffer,_In_ UINT32 PropertyBufferSizeInBytes,_Out_opt_ UINT32 * WrittenSizeInBytes) {
	Logs("WHvGetPartitionProperty Called");
	return _WHvGetPartitionProperty(Partition,PropertyCode,PropertyBuffer,PropertyBufferSizeInBytes, WrittenSizeInBytes);
}
HRESULT WINAPI WHvSetPartitionProperty(_In_ WHV_PARTITION_HANDLE Partition,_In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,_In_reads_bytes_(PropertyBufferSizeInBytes) const VOID * PropertyBuffer,_In_ UINT32 PropertyBufferSizeInBytes) {
	//TODO: Move this to Setup Partition other wise it might be called multiple times
	WHV_PARTITION_PROPERTY* props = (WHV_PARTITION_PROPERTY*)PropertyBuffer;
	if (props->ProcessorCount == 4)props->ProcessorCount = 5;
	
	return _WHvSetPartitionProperty(Partition, PropertyCode, PropertyBuffer, PropertyBufferSizeInBytes);
}
HRESULT WINAPI WHvMapGpaRange(_In_ WHV_PARTITION_HANDLE Partition, _In_ VOID * SourceAddress, _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, _In_ UINT64 SizeInBytes, _In_ WHV_MAP_GPA_RANGE_FLAGS Flags) {
	//Logs("WHvMapGpaRange Called " + std::to_string(SizeInBytes)+" at "+std::to_string(GuestAddress));
	HRESULT hr;
	if (ramLocInHost == nullptr) {
		if (GuestAddress == 0) {
			ramLocInHost = SourceAddress;
			ramSize = SizeInBytes;
		}
	}
	hr = _WHvMapGpaRange(Partition, SourceAddress, GuestAddress, SizeInBytes, Flags);
	return hr;
}
HRESULT WINAPI WHvUnmapGpaRange(_In_ WHV_PARTITION_HANDLE Partition,_In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,_In_ UINT64 SizeInBytes) {
	//Logs("WHvUnmapGpaRange Called");
	return _WHvUnmapGpaRange(Partition, GuestAddress, SizeInBytes);
}
HRESULT WINAPI WHvTranslateGva(_In_ WHV_PARTITION_HANDLE Partition,_In_ UINT32 VpIndex,_In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,_In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags,_Out_ WHV_TRANSLATE_GVA_RESULT* TranslationResult,_Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa) {
	//Logs("WHvTranslateGva Called");
	HRESULT HR = _WHvTranslateGva(Partition, VpIndex, Gva, TranslateFlags, TranslationResult, Gpa);
	if (resultAddress == 0) {
		WHV_REGISTER_NAME r = WHV_REGISTER_NAME::WHvX64RegisterCr3;
		WHV_REGISTER_VALUE val;
		if (_WHvGetVirtualProcessorRegisters(Partition, VpIndex, &r, 1, &val) != S_OK) {
			Logs("NoRegister");
			return HR;
		}
		WHV_TRANSLATE_GVA_RESULT res;
		WHV_GUEST_PHYSICAL_ADDRESS addr;
		if (GivenAddress != 0) {
			HRESULT hx = _WHvTranslateGva(Partition, VpIndex, GivenAddress, TranslateFlags, &res, &addr);
			if (res.ResultCode == WHV_TRANSLATE_GVA_RESULT_CODE::WHvTranslateGvaResultSuccess && hx == S_OK) {
				/*Logs("Done " + to_hex(addr));
				Logs("CR3 " + to_hex(val.Reg64));
				Logs("GVA " + to_hex(Gva));*/
				Logs("GVA " + to_hex(Gva));
				resultAddress = addr;
				if (!TranslateOnly) {
					GpaMemoryArea* area = new GpaMemoryArea;
					ChangeMemoryPerms(addr, 0x100, *area);
					hookRegions.push_back(area);
				}
			}
		}
	}
	return HR;
}
//HRESULT WINAPI WHvQueryGpaRangeDirtyBitmap(_In_ WHV_PARTITION_HANDLE Partition, _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, _In_ UINT64 RangeSizeInBytes, _Out_writes_bytes_opt_(BitmapSizeInBytes) UINT64 * Bitmap, _In_ UINT32 BitmapSizeInBytes) {
//	//Logs("WHvQueryGpaRangeDirtyBitmap Called");
//	return _WHvQueryGpaRangeDirtyBitmap(Partition, GuestAddress, RangeSizeInBytes, Bitmap, BitmapSizeInBytes);
//}
HRESULT WINAPI WHvCreateVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _In_ UINT32 Flags) {
	//Logs("WHvCreateVirtualProcessor Called");
	VPindexs.push_back(VpIndex);
	return _WHvCreateVirtualProcessor(Partition, VpIndex, Flags);
}
HRESULT WINAPI WHvDeleteVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex) {
	//Logs("WHvDeleteVirtualProcessor Called");
	return _WHvDeleteVirtualProcessor(Partition, VpIndex);
}
void RunNewVP(GpaMemoryArea* area,UINT32 VpIndex) {
	_EnableSingleStep(VpIndex);
	RevertMemoryPerms(*area);
	WHV_RUN_VP_EXIT_CONTEXT ExitContext = {};
	WHV_REGISTER_NAME name = WHV_REGISTER_NAME::WHvX64RegisterRip;
	WHV_REGISTER_VALUE val;
	_WHvGetVirtualProcessorRegisters(HookPartition, VpIndex, &name, 1, &val);
	UINT64 ripPhysical;
	WHV_TRANSLATE_GVA_RESULT res;
	_WHvTranslateGva(HookPartition, VpIndex, val.Reg64, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateExecute, &res, &ripPhysical);
	while (ripPhysical > area->gpa_start && ripPhysical < area->gpa_end) {
		_WHvRunVirtualProcessor(HookPartition, VpIndex, &ExitContext, sizeof(ExitContext));
		if (ExitContext.ExitReason == WHvRunVpExitReasonException && ExitContext.VpException.ExceptionType == WHV_EXCEPTION_TYPE::WHvX64ExceptionTypeDebugTrapOrFault) {
			val.Reg64 = ExitContext.VpContext.Rip;
			_WHvTranslateGva(HookPartition, VpIndex, val.Reg64, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateExecute, &res, &ripPhysical);
		}
		else {
			Logs("Hook VP failes");
		}
	}
	_DisableSingleStep(VpIndex);
	ChangeMemoryPerms(*area);
}
HRESULT WINAPI WHvRunVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext, _In_ UINT32 ExitContextSizeInBytes) {
	HRESULT hr;
	RUN_VP:
	hr = _WHvRunVirtualProcessor(Partition, VpIndex, ExitContext, ExitContextSizeInBytes);	
	auto ex = (WHV_RUN_VP_EXIT_CONTEXT*)ExitContext;
	//Memory access violation
	if (ex->ExitReason == WHvRunVpExitReasonMemoryAccess&& ex->MemoryAccess.AccessInfo.AccessType==WHvMemoryAccessExecute) {
		for (int i = 0; i < hookRegions.size(); i++) {
			//hook sanity check
			if (hookRegions[i]->host_ptr != nullptr) {
				//violation is for hooked GPA
				if (ex->MemoryAccess.Gpa >= hookRegions[i]->gpa_start && ex->MemoryAccess.Gpa < hookRegions[i]->gpa_end) {
					{
						std::lock_guard<std::mutex> lock(hookCriticalMutex);
						WHV_TRANSLATE_GVA_RESULT currRes;
						WHV_GUEST_PHYSICAL_ADDRESS gpaddr;
						_WHvTranslateGva(HookPartition, VpIndex, ex->VpContext.Rip, WHV_TRANSLATE_GVA_FLAGS::WHvTranslateGvaFlagValidateExecute, &currRes, &gpaddr);
						if (currRes.ResultCode != WHV_TRANSLATE_GVA_RESULT_CODE::WHvTranslateGvaResultSuccess || hookRegions[i] == nullptr) {
							continue;
						}
						if (hookRegions[i]->actual_addr == gpaddr) {
							CustomLogic(VpIndex);
						}
						
						TransferCPUCtx(VpIndex, regIndex);
						RunNewVP(hookRegions[i], regIndex);
						TransferCPUCtx(regIndex, VpIndex);
					}
					goto RUN_VP;
				}
			}
		}
	}
	return hr;
}
HRESULT WINAPI WHvCancelRunVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _In_ UINT32 Flags) {
	//Logs("WHvCancelRunVirtualProcessor Called");
	return _WHvCancelRunVirtualProcessor(Partition, VpIndex, Flags);
}
HRESULT WINAPI WHvGetVirtualProcessorRegisters(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames, _In_ UINT32 RegisterCount, _Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues) {
	//Logs("WHvGetVirtualProcessorRegisters Called");
	HRESULT hr = _WHvGetVirtualProcessorRegisters(Partition, VpIndex, RegisterNames, RegisterCount, RegisterValues);
	
	return hr;
}
HRESULT WINAPI WHvGetVirtualProcessorXsaveState(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _Out_writes_bytes_to_(BufferSizeInBytes, *BytesWritten) VOID* Buffer, _In_ UINT32 BufferSizeInBytes, _Out_ UINT32* BytesWritten) {
	//Logs("WHvGetVirtualProcessorXsaveState Called");
	return _WHvGetVirtualProcessorXsaveState(Partition, VpIndex, Buffer, BufferSizeInBytes, BytesWritten);
}
HRESULT WINAPI WHvSetVirtualProcessorRegisters(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames, _In_ UINT32 RegisterCount, _In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues) {
	//Logs("WHvSetVirtualProcessorRegisters Called");
	return _WHvSetVirtualProcessorRegisters(Partition, VpIndex, RegisterNames, RegisterCount, RegisterValues);
}
HRESULT WINAPI WHvSetVirtualProcessorXsaveState(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _In_reads_bytes_(BufferSizeInBytes) const VOID* Buffer, _In_ UINT32 BufferSizeInBytes) {
	//Logs("WHvSetVirtualProcessorXsaveState Called");
	return _WHvSetVirtualProcessorXsaveState(Partition, VpIndex, Buffer, BufferSizeInBytes);
}
HRESULT WINAPI WHvGetVirtualProcessorInterruptControllerState(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State, _In_ UINT32 StateSize, _Out_opt_ UINT32* WrittenSize) {
	//Logs("WHvGetVirtualProcessorInterruptControllerState Called");
	return _WHvGetVirtualProcessorInterruptControllerState(Partition, VpIndex, State, StateSize, WrittenSize);
}
HRESULT WINAPI WHvRequestInterrupt(_In_ WHV_PARTITION_HANDLE Partition, _In_ const WHV_INTERRUPT_CONTROL* Interrupt, _In_ UINT32 InterruptControlSize) {
	//Logs("WHvRequestInterrupt Called");
	return _WHvRequestInterrupt(Partition, Interrupt, InterruptControlSize);
}
HRESULT WINAPI WHvSetVirtualProcessorInterruptControllerState(_In_ WHV_PARTITION_HANDLE Partition,_In_ UINT32 VpIndex,_In_reads_bytes_(StateSize) const VOID* State,_In_ UINT32 StateSize) {
	//Logs("WHvSetVirtualProcessorInterruptControllerState Called");
	return _WHvSetVirtualProcessorInterruptControllerState(Partition, VpIndex, State, StateSize);
}
HRESULT WINAPI WHvGetPartitionCounters(_In_ WHV_PARTITION_HANDLE Partition, _In_ WHV_PARTITION_COUNTER_SET CounterSet, _Out_writes_bytes_to_(BufferSizeInBytes, *BytesWritten) VOID* Buffer, _In_ UINT32 BufferSizeInBytes, _Out_opt_ UINT32* BytesWritten) {
	//Logs("WHvGetPartitionCounters Called");
	return _WHvGetPartitionCounters(Partition, CounterSet, Buffer, BufferSizeInBytes, BytesWritten);
}
HRESULT WINAPI WHvGetVirtualProcessorCounters(_In_ WHV_PARTITION_HANDLE Partition, _In_ UINT32 VpIndex, _In_ WHV_PROCESSOR_COUNTER_SET CounterSet, _Out_writes_bytes_to_(BufferSizeInBytes, *BytesWritten) VOID* Buffer, _In_ UINT32 BufferSizeInBytes, _Out_opt_ UINT32* BytesWritten) {
	//Logs("WHvGetVirtualProcessorCounters Called");
	return _WHvGetVirtualProcessorCounters(Partition, VpIndex, CounterSet, Buffer, BufferSizeInBytes, BytesWritten);
}
HRESULT WINAPI WHvGetVirtualProcessorInterruptControllerState2(_In_ WHV_PARTITION_HANDLE Partition,_In_ UINT32 VpIndex,_Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State,_In_ UINT32 StateSize,_Out_opt_ UINT32* WrittenSize) {
	return _WHvGetVirtualProcessorInterruptControllerState2(Partition, VpIndex, State, StateSize, WrittenSize);
}
HRESULT WINAPI WHvSetVirtualProcessorInterruptControllerState2(_In_ WHV_PARTITION_HANDLE Partition,_In_ UINT32 VpIndex,_In_reads_bytes_(StateSize) const VOID* State,_In_ UINT32 StateSize) {
	return _WHvSetVirtualProcessorInterruptControllerState2(Partition, VpIndex, State, StateSize);
}
HRESULT WINAPI WHvSuspendPartitionTime(_In_ WHV_PARTITION_HANDLE Partition) {
	return _WHvSuspendPartitionTime(Partition);
}
HRESULT WINAPI WHvResumePartitionTime(_In_ WHV_PARTITION_HANDLE Partition) {
	return _WHvResumePartitionTime(Partition);
}
void Init() {
	//Logs("Started");
	lib = LoadLibraryA("C:\\Windows\\System32\\WinHvPlatform.dll");
	GivenAddress = 0;
	FARPROC proc = GetProcAddress(lib, "WHvGetCapability");
	_WHvGetCapability = (WHvGetCapability_t)proc;
	proc = GetProcAddress(lib, "WHvCreatePartition");
	_WHvCreatePartition = (WHvCreatePartition_t)proc;

	proc = GetProcAddress(lib, "WHvSetupPartition");
	_WHvSetupPartition = (WHvSetupPartition_t)proc;

	proc = GetProcAddress(lib, "WHvDeletePartition");
	_WHvDeletePartition = (WHvDeletePartition_t)proc;

	proc = GetProcAddress(lib, "WHvGetPartitionProperty");
	_WHvGetPartitionProperty = (WHvGetPartitionProperty_t)proc;

	proc = GetProcAddress(lib, "WHvSetPartitionProperty");
	_WHvSetPartitionProperty = (WHvSetPartitionProperty_t)proc;

	proc = GetProcAddress(lib, "WHvMapGpaRange");
	_WHvMapGpaRange = (WHvMapGpaRange_t)proc;

	proc = GetProcAddress(lib, "WHvUnmapGpaRange");
	_WHvUnmapGpaRange = (WHvUnmapGpaRange_t)proc;

	proc = GetProcAddress(lib, "WHvTranslateGva");
	_WHvTranslateGva = (WHvTranslateGva_t)proc;

	proc = GetProcAddress(lib, "WHvQueryGpaRangeDirtyBitmap");
	_WHvQueryGpaRangeDirtyBitmap = (WHvQueryGpaRangeDirtyBitmap_t)proc;

	proc = GetProcAddress(lib, "WHvCreateVirtualProcessor");
	_WHvCreateVirtualProcessor = (WHvCreateVirtualProcessor_t)proc;

	proc = GetProcAddress(lib, "WHvDeleteVirtualProcessor");
	_WHvDeleteVirtualProcessor = (WHvDeleteVirtualProcessor_t)proc;

	proc = GetProcAddress(lib, "WHvRunVirtualProcessor");
	_WHvRunVirtualProcessor = (WHvRunVirtualProcessor_t)proc;

	proc = GetProcAddress(lib, "WHvCancelRunVirtualProcessor");
	_WHvCancelRunVirtualProcessor = (WHvCancelRunVirtualProcessor_t)proc;

	proc = GetProcAddress(lib, "WHvGetVirtualProcessorRegisters");
	_WHvGetVirtualProcessorRegisters = (WHvGetVirtualProcessorRegisters_t)proc;

	proc = GetProcAddress(lib, "WHvGetVirtualProcessorXsaveState");
	_WHvGetVirtualProcessorXsaveState = (WHvGetVirtualProcessorXsaveState_t)proc;

	proc = GetProcAddress(lib, "WHvSetVirtualProcessorRegisters");
	_WHvSetVirtualProcessorRegisters = (WHvSetVirtualProcessorRegisters_t)proc;

	proc = GetProcAddress(lib, "WHvSetVirtualProcessorXsaveState");
	_WHvSetVirtualProcessorXsaveState = (WHvSetVirtualProcessorXsaveState_t)proc;

	proc = GetProcAddress(lib, "WHvGetVirtualProcessorInterruptControllerState");
	_WHvGetVirtualProcessorInterruptControllerState = (WHvGetVirtualProcessorInterruptControllerState_t)proc;

	proc = GetProcAddress(lib, "WHvRequestInterrupt");
	_WHvRequestInterrupt = (WHvRequestInterrupt_t)proc;

	proc = GetProcAddress(lib, "WHvSetVirtualProcessorInterruptControllerState");
	_WHvSetVirtualProcessorInterruptControllerState = (WHvSetVirtualProcessorInterruptControllerState_t)proc;

	proc = GetProcAddress(lib, "WHvGetPartitionCounters");
	_WHvGetPartitionCounters = (WHvGetPartitionCounters_t)proc;

	proc = GetProcAddress(lib, "WHvGetVirtualProcessorCounters");
	_WHvGetVirtualProcessorCounters = (WHvGetVirtualProcessorCounters_t)proc;

	proc = GetProcAddress(lib, "WHvGetVirtualProcessorInterruptControllerState2");
	_WHvGetVirtualProcessorInterruptControllerState2 = (WHvGetVirtualProcessorInterruptControllerState2_t)proc;

	proc = GetProcAddress(lib, "WHvSetVirtualProcessorInterruptControllerState2");
	_WHvSetVirtualProcessorInterruptControllerState2 = (WHvSetVirtualProcessorInterruptControllerState2_t)proc;

	proc = GetProcAddress(lib, "WHvSuspendPartitionTime");
	_WHvSuspendPartitionTime = (WHvSuspendPartitionTime_t)proc;

	proc = GetProcAddress(lib, "WHvResumePartitionTime");
	_WHvResumePartitionTime = (WHvResumePartitionTime_t)proc;
}
void DeInit() {
	FreeLibrary(lib);
	//Logs("Unloaded");
}
