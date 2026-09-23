#pragma once

#ifndef _AMD64_
#define _AMD64_
#endif
#ifndef AMD64
#define AMD64
#endif
#ifndef _WIN64
#define _WIN64
#endif
#ifndef _KERNEL_MODE
#define _KERNEL_MODE 1
#endif

#include <sal.h>
#include <driverspecs.h>
#include <ntddk.h>
#include <impl/std/std.h>
#include <impl/ia32/ia32.h>

#include <dependencies/oxorany/include.h>

#include <workspace/kernel/ntoskrnl/symbols.h>
#include <workspace/kernel/ntoskrnl/ntoskrnl.hxx>
#include <workspace/kernel/ntoskrnl/exports/exports.hxx>

#include <workspace/kernel/core/mmu/mmu.hxx>

#include <workspace/kernel/paging/pagetables.h>
#include <workspace/kernel/core/physical/physical.hxx>
#include <workspace/kernel/core/process/process.hxx>
#include <workspace/kernel/core/hide/hide.hxx>

#include <workspace/kernel/paging/paging.hxx>
#include <workspace/kernel/paging/dpm/dpm.hxx>
#include <workspace/kernel/paging/pfn_cr3/pfn_cr3.hxx>

#include <workspace/client/control.h>
#include <workspace/client/client.hxx>
