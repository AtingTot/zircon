// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <fbl/macros.h>
#include <fvm/fvm.h>
#include <zircon/status.h>
#include <zircon/types.h>

#include "crypto/utils.h"

#define DEFINE_EACH_DEVICE(Test)                                                                   \
    bool Test##Raw(Superblock::Version version) { return Test(version, false /* not FVM */); }     \
    DEFINE_EACH(Test##Raw);                                                                        \
    bool Test##Fvm(Superblock::Version version) { return Test(version, true /* FVM */); }          \
    DEFINE_EACH(Test##Fvm);

#define RUN_EACH_DEVICE(Test)                                                                      \
    RUN_EACH(Test##Raw)                                                                            \
    RUN_EACH(Test##Fvm)

namespace zxcrypt {
namespace testing {

// Default disk geometry to use when testing device block related code.
const uint32_t kBlockCount = 64;
const uint32_t kBlockSize = 512;
const size_t kDeviceSize = kBlockCount * kBlockSize;
const uint32_t kSliceCount = (kBlockCount * kBlockSize) / FVM_BLOCK_SIZE;

// |zxcrypt::testing::Utils| is a collection of functions designed to make the zxcrypt
// unit test setup and tear down easier.
class TestDevice final {
public:
    explicit TestDevice();
    ~TestDevice();

    // Returns a duplicated file descriptor representing the zxcrypt volume's underlying device;
    // that is, the ramdisk or FVM partition.
    fbl::unique_fd parent() const {
        return fbl::unique_fd(dup(fvm_part_ ? fvm_part_.get() : ramdisk_.get()));
    }

    // Returns the block size of the zxcrypt device.
    size_t block_size() const { return block_size_; }

    // Returns a reference to the root key generated for this device.
    const crypto::Bytes& key() const { return key_; }

    // Allocates a new block device of at least |device_size| bytes grouped into blocks of
    // |block_size| bytes each.  If |fvm| is true, it will be formatted as an FVM partition with the
    // appropriates number of slices of |FVM_BLOCK_SIZE| each.  A file descriptor for the block
    // device is returned via |out_fd|.
    zx_status_t Create(size_t device_size, size_t block_size, bool fvm);

    // Generates a key of an appropriate length for the given |version|.
    zx_status_t GenerateKey(Superblock::Version version);

    // Convenience method that generates a key and creates a device according to |version| and
    // |fvm|.
    zx_status_t DefaultInit(Superblock::Version version, bool fvm);

    // Flips a (pseudo)random bit in the byte at the given |offset| on the block device.  The call
    // to |srand| in main.c guarantees the same bit will be chosen for a given test iteration.
    zx_status_t Corrupt(zx_off_t offset);

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(TestDevice);

    // Allocates a new ramdisk of at least |device_size| bytes arranged into blocks of |block_size|
    // bytes, and opens it.
    zx_status_t CreateRamdisk(size_t device_size, size_t block_size);

    // Creates a ramdisk of with enough blocks of |block_size| bytes to hold both FVM metadata and
    // an FVM partition of at least |device_size| bytes.  It formats the ramdisk to be an FVM
    // device, and allocates a partition with a single slice of size FVM_BLOCK_SIZE.
    zx_status_t CreateFvmPart(size_t device_size, size_t block_size);

    // Seeks to the given |offset| in |fd|, and writes |length| bytes from |buf|.
    zx_status_t Write(const fbl::unique_fd& fd, const uint8_t* buf, zx_off_t offset, size_t length);

    // Seeks to the given |offset| in |fd|, and reads |length| bytes into |buf|.
    zx_status_t Read(const fbl::unique_fd& fd, uint8_t* buf, zx_off_t offset, size_t length);

    // Tears down the current ramdisk and all its children.
    void Reset();

    // The pathname of the ramdisk
    char ramdisk_path_[PATH_MAX];
    // The pathname of the FVM partition.
    char fvm_part_path_[PATH_MAX];
    // File descriptor for the underlying ramdisk.
    fbl::unique_fd ramdisk_;
    // File descriptor for the (optional) underlying FVM partition.
    fbl::unique_fd fvm_part_;
    // The cached block count.
    size_t block_count_;
    // The cached block size.
    size_t block_size_;
    // The root key for this device.
    crypto::Bytes key_;
    // An internal write buffer, initially filled with pseudo-random data
    fbl::unique_ptr<uint8_t[]> to_write_;
    // An internal write buffer,  initially filled with zeros.
    fbl::unique_ptr<uint8_t[]> as_read_;
};

} // namespace testing
} // namespace zxcrypt
