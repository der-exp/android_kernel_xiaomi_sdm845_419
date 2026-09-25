/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Библиотечный интерфейс ChaCha (chacha_init/chacha20_crypt, как в ядрах 5.5+)
 * для старых ядер, где его нет: в 4.9 <crypto/chacha.h> либо отсутствует вовсе
 * (upstream 4.9 LTS), либо это заголовок бэкпорта Adiantum с crypto_chacha_init
 * и struct chacha_ctx (ядра Android), и в обоих случаях защите заголовков
 * AmneziaWG (header_protection.c, send.c, receive.c, socket.c) опереться не на
 * что. Kbuild подкладывает этот файл впереди системного, если в системном нет
 * chacha20_crypt, и всегда, когда модуль собирает zinc (ядра до 5.10): тогда
 * библиотечный заголовок ядра, даже если он есть (Android 4.19 с бэкпортом
 * WireGuard), своими макросами CHACHA20_* ломает zinc/chacha20.h.
 *
 * Поток шифра берётся у zinc, который на ядрах до 5.10 и так собирается в
 * модуль для chacha20poly1305. Раскладка состояния совпадает с библиотечной:
 * u32[16], где 0..3 — константы, 4..11 — ключ, 12 — счётчик блоков, 13..15 —
 * нонс; struct chacha20_ctx у zinc — то же самое объединение над u32[16], и
 * счётчик в state[12] zinc продвигает так же, на блок за каждые начатые 64
 * байта. Поэтому receive.c может, как и на новых ядрах, отмотать счётчик
 * записью state[12] = 0.
 *
 * SIMD не используется нарочно (HAVE_NO_SIMD): защищаются только заголовки,
 * это единицы и сотни байт, а NEON у zinc включается с трёх блоков; сохранять
 * и восстанавливать регистры FPU ради одного-двух блоков дороже, чем посчитать
 * их скалярной реализацией.
 */
#ifndef _COMPAT_CRYPTO_CHACHA_H
#define _COMPAT_CRYPTO_CHACHA_H

#include <zinc/chacha20.h>
#include <linux/simd.h>
#include <linux/types.h>
#include <asm/unaligned.h>

#define CHACHA_IV_SIZE		16
#define CHACHA_KEY_SIZE		32
#define CHACHA_BLOCK_SIZE	64
#define CHACHA_STATE_WORDS	(CHACHA_BLOCK_SIZE / sizeof(u32))

static inline void chacha_init(u32 *state, const u32 *key, const u8 *iv)
{
	state[0] = CHACHA20_CONSTANT_EXPA;
	state[1] = CHACHA20_CONSTANT_ND_3;
	state[2] = CHACHA20_CONSTANT_2_BY;
	state[3] = CHACHA20_CONSTANT_TE_K;
	state[4] = key[0];
	state[5] = key[1];
	state[6] = key[2];
	state[7] = key[3];
	state[8] = key[4];
	state[9] = key[5];
	state[10] = key[6];
	state[11] = key[7];
	state[12] = get_unaligned_le32(iv + 0);
	state[13] = get_unaligned_le32(iv + 4);
	state[14] = get_unaligned_le32(iv + 8);
	state[15] = get_unaligned_le32(iv + 12);
}

static inline void chacha20_crypt(u32 *state, u8 *dst, const u8 *src,
				  unsigned int bytes)
{
	simd_context_t simd_context = HAVE_NO_SIMD;

	BUILD_BUG_ON(sizeof(struct chacha20_ctx) != CHACHA_STATE_WORDS * sizeof(u32));
	chacha20((struct chacha20_ctx *)state, dst, src, bytes, &simd_context);
}

#endif /* _COMPAT_CRYPTO_CHACHA_H */
