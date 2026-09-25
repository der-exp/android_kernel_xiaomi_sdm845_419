# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2015-2019 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.

ccflags-y := -D'pr_fmt(fmt)=KBUILD_MODNAME ": " fmt'
ccflags-y += -Wframe-larger-than=2048
ccflags-$(CONFIG_AMNEZIAWG_DEBUG) += -DDEBUG -g
ccflags-$(if $(WIREGUARD_VERSION),y,) += -D'WIREGUARD_VERSION="$(WIREGUARD_VERSION)"'
ccflags-$(if $(OMIT_ENDPOINTS),y,) += -D'OMIT_ENDPOINTS="$(OMIT_ENDPOINTS)"'

amneziawg-y := main.o noise.o device.o peer.o timers.o queueing.o send.o receive.o socket.o peerlookup.o allowedips.o ratelimiter.o cookie.o netlink.o junk.o header_protection.o

ifeq ($(shell [ $(VERSION) -lt 6 -o \( $(VERSION) -eq 6 -a $(PATCHLEVEL) -lt 1 \) ] && echo y),y)
	kbuild-dir := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
else
	kbuild-dir := $(src)
endif

# Встроенный модуль: приставка awg_ для всех глобальных имён (см. ниже). Подключается
# раньше compat.h, потому что compat.h сам включает заголовки zinc с их объявлениями.
ifeq ($(if $(KBUILD_EXTMOD),,$(CONFIG_AMNEZIAWG)),y)
ccflags-y += -include $(kbuild-dir)/builtin-rename.h
asflags-y += -include $(kbuild-dir)/builtin-rename.h
endif

ifeq ($(shell [ $(VERSION) -lt 5 -o \( $(VERSION) -eq 5 -a $(PATCHLEVEL) -lt 10 \) ] && echo y),y)
	include $(src)/crypto/Kbuild.include
endif
include $(src)/compat/Kbuild.include

obj-$(if $(KBUILD_EXTMOD),m,$(CONFIG_AMNEZIAWG)) := amneziawg.o

# Встроенный в ядро (=y) модуль живёт в одном vmlinux со встроенным WireGuard, а у них общая
# родословная: одни и те же глобальные функции (wg_device_init, wg_noise_init, …), а zinc
# модуля определяет те же имена, что и lib/crypto ядра под WireGuard (chacha20poly1305_*,
# poly1305_blocks_neon, …), и компоновщик находит каждое имя дважды. Наружу модуль ничего
# не отдаёт — EXPORT_SYMBOL в нём нет, а ядро вызывает его только через initcall и таблицы
# операций, — поэтому все его глобальные имена получают приставку awg_ через
# builtin-rename.h, который подключается к каждому исходнику. Сами исходники при этом не
# трогаются, и обновление модуля — это снова копия его src/ без переименований.
#
# Делать символы локальными objcopy после сборки, как на 4.9, здесь нельзя: с
# CONFIG_LTO_CLANG объекты — биткод LLVM, и именно в нём, на этапе LTO всего vmlinux,
# компоновщик встречает пары имён; а составной amneziawg.o 4.19 не собирает вовсе, части
# модуля ложатся прямо в built-in.a. Отдельная предварительная LTO-сборка модуля тоже не
# годится: с CONFIG_CFI_CLANG таблицы переходов модуля должны быть частью таблиц vmlinux,
# иначе косвенный вызов ядра в ndo_start_xmit и прочие обработчики модуля не пройдёт
# проверку CFI.
#
# Если после обновления модуля в нём появится новое глобальное имя, проверка ниже
# остановит сборку и напечатает строки, которые нужно добавить в builtin-rename.h.
#
# Для модуля (=m) и сборки вне дерева не нужно: у модулей свои пространства имён, а
# init_module/cleanup_module должны остаться глобальными.
ifeq ($(if $(KBUILD_EXTMOD),,$(CONFIG_AMNEZIAWG)),y)
awg-nm := $(if $(CONFIG_LTO_CLANG),$(LLVM_NM),$(NM))
quiet_cmd_awg_symcheck = CHECK   $@
      cmd_awg_symcheck = \
	syms=$$($(awg-nm) --defined-only --extern-only $(filter %.o,$^) | \
		awk 'NF == 3 { print $$3 }' | \
		grep -v -e '^awg_' -e '^__cfi_check' -e '\.' | sort -u); \
	if [ -n "$$syms" ]; then \
		echo "$(src): global symbols without the awg_ prefix, add to builtin-rename.h:" >&2; \
		for s in $$syms; do echo "\#define $$s awg_$${s\#wg_}" >&2; done; \
		exit 1; \
	fi; \
	touch $@

$(obj)/builtin-rename.chk: $(addprefix $(obj)/,$(amneziawg-y)) FORCE
	$(call if_changed,awg_symcheck)

always += builtin-rename.chk
targets += builtin-rename.chk
endif
