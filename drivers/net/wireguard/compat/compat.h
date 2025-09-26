/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2015-2019 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#ifndef _WG_COMPAT_H
#define _WG_COMPAT_H

#include <linux/ktime.h>
#include <linux/timekeeping.h>
static inline u64 ktime_get_coarse_boottime_ns(void)
{
	return ktime_to_ns(ktime_mono_to_any(ns_to_ktime(jiffies64_to_nsecs(get_jiffies_64())), TK_OFFS_BOOT));
}

#include <linux/vmalloc.h>
#include <linux/mm.h>
static inline void *__compat_kvcalloc(size_t n, size_t size, gfp_t flags)
{
        return kvmalloc_array(n, size, flags | __GFP_ZERO);
}
#define kvcalloc __compat_kvcalloc

#include <net/genetlink.h>
#define genl_dump_check_consistent(a, b) genl_dump_check_consistent(a, b, &genl_family)

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0) && !defined(ISRHEL7)
static inline void *skb_put_data(struct sk_buff *skb, const void *data, unsigned int len)
{
	void *tmp = skb_put(skb, len);
	memcpy(tmp, data, len);
	return tmp;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) && !defined(ISRHEL7)
#define napi_complete_done(n, work_done) napi_complete(n)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
#include <linux/netdevice.h>
/* NAPI_STATE_SCHED gets set by netif_napi_add anyway, so this is safe.
 * Also, kernels without NAPI_STATE_NO_BUSY_POLL don't have a call to
 * napi_hash_add inside of netif_napi_add.
 */
#define NAPI_STATE_NO_BUSY_POLL NAPI_STATE_SCHED
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)
#include <linux/atomic.h>
#ifndef atomic_read_acquire
#define atomic_read_acquire(v) ({ int __compat_p1 = atomic_read(v); smp_rmb(); __compat_p1; })
#endif
#ifndef atomic_set_release
#define atomic_set_release(v, i) ({ smp_wmb(); atomic_set(v, i); })
#endif
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 3, 0)
#include <linux/atomic.h>
#ifndef atomic_read_acquire
#define atomic_read_acquire(v) smp_load_acquire(&(v)->counter)
#endif
#ifndef atomic_set_release
#define atomic_set_release(v, i) smp_store_release(&(v)->counter, (i))
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 285)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 320))
static inline void le32_to_cpu_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__le32_to_cpus(buf);
		buf++;
	}
}
static inline void cpu_to_le32_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__cpu_to_le32s(buf);
		buf++;
	}
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
#include <crypto/algapi.h>
static inline void crypto_xor_cpy(u8 *dst, const u8 *src1, const u8 *src2,
				  unsigned int size)
{
	if (IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) &&
	    __builtin_constant_p(size) &&
	    (size % sizeof(unsigned long)) == 0) {
		unsigned long *d = (unsigned long *)dst;
		unsigned long *s1 = (unsigned long *)src1;
		unsigned long *s2 = (unsigned long *)src2;

		while (size > 0) {
			*d++ = *s1++ ^ *s2++;
			size -= sizeof(unsigned long);
		}
	} else {
		if (unlikely(dst != src1))
			memmove(dst, src1, size);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
		crypto_xor(dst, src2, size);
#else
		__crypto_xor(dst, src2, size);
#endif
	}
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#define read_cpuid_part() read_cpuid_part_number()
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0) && !defined(ISRHEL7)
#define hlist_add_behind(a, b) hlist_add_after(b, a)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0) && !defined(ISRHEL8)
#define totalram_pages() totalram_pages
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
struct __kernel_timespec {
	int64_t tv_sec, tv_nsec;
};
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
#include <linux/time64.h>
#ifdef __kernel_timespec
#undef __kernel_timespec
struct __kernel_timespec {
	int64_t tv_sec, tv_nsec;
};
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#include <linux/kernel.h>
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) __ALIGN_KERNEL((x) - ((a) - 1), (a))
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0) && !defined(ISRHEL8)
#include <linux/skbuff.h>
#define skb_probe_transport_header(a) skb_probe_transport_header(a, 0)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0) && !defined(ISRHEL7)
#define ignore_df local_df
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0) && !defined(ISRHEL8)
/* Note that all intentional uses of the non-_bh variety need to explicitly
 * undef these, conditionalized on COMPAT_CANNOT_DEPRECIATE_BH_RCU.
 */
#include <linux/rcupdate.h>
static __always_inline void old_synchronize_rcu(void)
{
	synchronize_rcu();
}
static __always_inline void old_call_rcu(void *a, void *b)
{
	call_rcu(a, b);
}
static __always_inline void old_rcu_barrier(void)
{
	rcu_barrier();
}
#ifdef synchronize_rcu
#undef synchronize_rcu
#endif
#ifdef call_rcu
#undef call_rcu
#endif
#ifdef rcu_barrier
#undef rcu_barrier
#endif
#define synchronize_rcu synchronize_rcu_bh
#define call_rcu call_rcu_bh
#define rcu_barrier rcu_barrier_bh
#define COMPAT_CANNOT_DEPRECIATE_BH_RCU

#if defined(__aarch64__)
#define cpu_have_named_feature(name) (elf_hwcap & (HWCAP_ ## name))
#endif

#define blake2s_init zinc_blake2s_init
#define blake2s_init_key zinc_blake2s_init_key
#define blake2s_update zinc_blake2s_update
#define blake2s_final zinc_blake2s_final
#define blake2s_hmac zinc_blake2s_hmac
#define chacha20 zinc_chacha20
#define hchacha20 zinc_hchacha20
#define chacha20poly1305_encrypt zinc_chacha20poly1305_encrypt
#define chacha20poly1305_encrypt_sg_inplace zinc_chacha20poly1305_encrypt_sg_inplace
#define chacha20poly1305_decrypt zinc_chacha20poly1305_decrypt
#define chacha20poly1305_decrypt_sg_inplace zinc_chacha20poly1305_decrypt_sg_inplace
#define xchacha20poly1305_encrypt zinc_xchacha20poly1305_encrypt
#define xchacha20poly1305_decrypt zinc_xchacha20poly1305_decrypt
#define curve25519 zinc_curve25519
#define curve25519_generate_secret zinc_curve25519_generate_secret
#define curve25519_generate_public zinc_curve25519_generate_public
#define poly1305_init zinc_poly1305_init
#define poly1305_update zinc_poly1305_update
#define poly1305_final zinc_poly1305_final
#define blake2s_compress_ssse3 zinc_blake2s_compress_ssse3
#define blake2s_compress_avx512 zinc_blake2s_compress_avx512
#define poly1305_init_arm zinc_poly1305_init_arm
#define poly1305_blocks_arm zinc_poly1305_blocks_arm
#define poly1305_emit_arm zinc_poly1305_emit_arm
#define poly1305_blocks_neon zinc_poly1305_blocks_neon
#define poly1305_emit_neon zinc_poly1305_emit_neon
#define poly1305_init_mips zinc_poly1305_init_mips
#define poly1305_blocks_mips zinc_poly1305_blocks_mips
#define poly1305_emit_mips zinc_poly1305_emit_mips
#define poly1305_init_x86_64 zinc_poly1305_init_x86_64
#define poly1305_blocks_x86_64 zinc_poly1305_blocks_x86_64
#define poly1305_emit_x86_64 zinc_poly1305_emit_x86_64
#define poly1305_emit_avx zinc_poly1305_emit_avx
#define poly1305_blocks_avx zinc_poly1305_blocks_avx
#define poly1305_blocks_avx2 zinc_poly1305_blocks_avx2
#define poly1305_blocks_avx512 zinc_poly1305_blocks_avx512
#define curve25519_neon zinc_curve25519_neon
#define hchacha20_ssse3 zinc_hchacha20_ssse3
#define chacha20_ssse3 zinc_chacha20_ssse3
#define chacha20_avx2 zinc_chacha20_avx2
#define chacha20_avx512 zinc_chacha20_avx512
#define chacha20_avx512vl zinc_chacha20_avx512vl
#define chacha20_mips zinc_chacha20_mips
#define chacha20_arm zinc_chacha20_arm
#define hchacha20_arm zinc_hchacha20_arm
#define chacha20_neon zinc_chacha20_neon

#endif /* _WG_COMPAT_H */
