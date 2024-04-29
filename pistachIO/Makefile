CC := gcc
LD := gcc

SRC_DIR		:= ./
BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -march=native $(DEBUG) #$(shell python3-config --includes)

INC 		:= -I/usr/include/ -I./drivers -I./entry -I./include
CFLAGS 		+= $(INC)

# Configurations
CONFIG 	= .config
include ${CONFIG}

ifeq ($(TEST_INLINE),y)
	CFLAGS	+= -DTEST_INLINE
endif

ifeq ($(CONFIG_DPDK),y)
	CFLAGS	+= -DCONFIG_DPDK
endif

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

LDFLAGS 	:= -L/usr/lib -lpthread -lrt -ldl -L/home/ubuntu/tinycc -ltcc #$(shell python3-config --ldflags)

# DPDK
ifeq ($(CONFIG_OCTEON),y)
	CFLAGS		+= -DCONFIG_OCTEON
	DPDK_DIR	= /root/dpdk-19.11
	DPDK_BUILD	= $(DPDK_DIR)/build
	CFLAGS 		+= -D_FILE_OFFSET_BITS=64 -fPIC -DRTE_MACHINE_CPUFLAG_NEON -DRTE_MACHINE_CPUFLAG_CRC32	\
					-DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PMULL -DRTE_MACHINE_CPUFLAG_SHA1	\
					-DRTE_MACHINE_CPUFLAG_SHA2	-DRTE_USE_FUNCTION_VERSIONING -DALLOW_EXPERIMENTAL_API	\
					-I$(DPDK_BUILD)/include -include $(DPDK_BUILD)/include/rte_config.h
	LDFLAGS 	+= 	-L$(DPDK_BUILD)/lib -Wl,-lrte_flow_classify -Wl,--whole-archive -Wl,-lrte_pipeline -Wl,--no-whole-archive	\
					-Wl,--whole-archive -Wl,-lrte_table -Wl,--no-whole-archive -Wl,--whole-archive -Wl,-lrte_port	\
					-Wl,--no-whole-archive -Wl,-lrte_pdump -Wl,-lrte_distributor -Wl,-lrte_ip_frag -Wl,-lrte_meter	\
					-Wl,-lrte_fib -Wl,-lrte_rib -Wl,-lrte_lpm -Wl,-lrte_acl -Wl,-lrte_jobstats -Wl,-lrte_metrics	\
					-Wl,-lrte_bitratestats -Wl,-lrte_latencystats -Wl,-lrte_power -Wl,-lrte_efd -Wl,-lrte_bpf	\
					-Wl,-lrte_ipsec -Wl,--whole-archive -Wl,-lrte_cfgfile -Wl,-lrte_gro -Wl,-lrte_gso -Wl,-lrte_hash	\
					-Wl,-lrte_member -Wl,-lrte_vhost -Wl,-lrte_kvargs -Wl,-lrte_mbuf -Wl,-lrte_net -Wl,-lrte_ethdev	\
					-Wl,-lrte_bbdev -Wl,-lrte_cryptodev -Wl,-lrte_security -Wl,-lrte_compressdev -Wl,-lrte_eventdev	\
					-Wl,-lrte_rawdev -Wl,-lrte_timer -Wl,-lrte_mempool -Wl,-lrte_stack -Wl,-lrte_mempool_ring	\
					-Wl,-lrte_mempool_octeontx2 -Wl,-lrte_ring -Wl,-lrte_pci -Wl,-lrte_eal -Wl,-lrte_cmdline	\
					-Wl,-lrte_reorder -Wl,-lrte_sched -Wl,-lrte_rcu -Wl,-lrte_common_cpt -Wl,-lrte_common_octeontx	\
					-Wl,-lrte_common_octeontx2 -Wl,-lrte_common_dpaax -Wl,-lrte_bus_pci -Wl,-lrte_bus_vdev	\
					-Wl,-lrte_bus_dpaa -Wl,-lrte_bus_fslmc -Wl,-lrte_mempool_bucket -Wl,-lrte_mempool_stack	\
					-Wl,-lrte_mempool_dpaa -Wl,-lrte_mempool_dpaa2 -Wl,-lrte_pmd_af_packet -Wl,-lrte_pmd_ark	\
					-Wl,-lrte_pmd_atlantic -Wl,-lrte_pmd_axgbe -Wl,-lrte_pmd_bnxt -Wl,-lrte_pmd_bond -Wl,-lrte_pmd_cxgbe	\
					-Wl,-lrte_pmd_dpaa -Wl,-lrte_pmd_dpaa2 -Wl,-lrte_pmd_e1000 -Wl,-lrte_pmd_ena -Wl,-lrte_pmd_enetc	\
					-Wl,-lrte_pmd_enic -Wl,-lrte_pmd_failsafe -Wl,-lrte_pmd_hinic -Wl,-lrte_pmd_hns3 -Wl,-lrte_pmd_i40e	\
					-Wl,-lrte_pmd_iavf -Wl,-lrte_pmd_ice -Wl,-lrte_pmd_ixgbe -Wl,-lrte_pmd_lio -Wl,-lrte_pmd_octeontx_ep	\
					-Wl,-lrte_pmd_memif -Wl,-lrte_pmd_armada_ep -Wl,-lrte_pmd_nfp -Wl,-lrte_pmd_null 	\
					-Wl,-lrte_pmd_octeontx2 -Wl,-lrte_pmd_pfe -Wl,-lrte_pmd_qede -Wl,-lrte_pmd_ring -Wl,-lrte_pmd_softnic	\
					-Wl,-lrte_pmd_tap -Wl,-lrte_pmd_thunderx_nicvf -Wl,-lrte_pmd_vdev_netvsc -Wl,-lrte_pmd_virtio	\
					-Wl,-lrte_pmd_vhost -Wl,-lrte_pmd_ifc -Wl,-lrte_pmd_vmxnet3_uio -Wl,-lrte_bus_vmbus -Wl,-lrte_pmd_netvsc	\
					-Wl,-lrte_pmd_bbdev_null -Wl,-lrte_pmd_bbdev_fpga_lte_fec -Wl,-lrte_pmd_bbdev_turbo_sw	\
					-Wl,-lrte_pmd_null_crypto -Wl,-lrte_pmd_nitrox -Wl,-lrte_pmd_octeontx_crypto -Wl,-lrte_pmd_octeontx2_crypto	\
					-Wl,-lrte_pmd_crypto_scheduler -Wl,-lrte_pmd_dpaa2_sec -Wl,-lrte_pmd_dpaa_sec -Wl,-lrte_pmd_caam_jr	\
					-Wl,-lrte_pmd_virtio_crypto -Wl,-lrte_pmd_octeontx_zip -Wl,-lrte_pmd_qat -Wl,-lrte_pmd_skeleton_event	\
					-Wl,-lrte_pmd_sw_event -Wl,-lrte_pmd_dsw_event -Wl,-lrte_pmd_octeontx_ssovf -Wl,-lrte_pmd_dpaa_event	\
					-Wl,-lrte_pmd_dpaa2_event -Wl,-lrte_mempool_octeontx -Wl,-lrte_pmd_octeontx -Wl,-lrte_pmd_octeontx2_event	\
					-Wl,-lrte_pmd_opdl_event -Wl,-lrte_rawdev_skeleton -Wl,-lrte_rawdev_dpaa2_cmdif -Wl,-lrte_rawdev_dpaa2_qdma	\
					-Wl,-lrte_bus_ifpga -Wl,-lrte_rawdev_ntb -Wl,-lrte_rawdev_octeontx2_dma -Wl,-lrte_rawdev_octeontx2_ep	\
					-Wl,--no-whole-archive -Wl,-lrt -Wl,-lm -Wl,-export-dynamic -Wl,--as-needed	\
					-ldl -lpthread -lm
else ifeq ($(CONFIG_BLUEFIELD2),y)
	CFLAGS		+= -DCONFIG_BLUEFIELD2
	LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
	LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
	CFLAGS 		+= $(LIBDPDK_CFLAGS)
	LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

	ifeq ($(CONFIG_DOCA),y)
		CFLAGS	+= -DCONFIG_DOCA
# DOCA
		LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca)
		LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca)
		CFLAGS 		+= $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
		LDFLAGS 	+= $(LIBDOCA_LDFLAGS)

		ifeq ($(CONFIG_DOCA_REGEX),y)
			CFLAGS	+= -DCONFIG_DOCA_REGEX
		endif
	endif
endif

# Project files
LIBAIO_SRCS	= printk/printk.c kernel/sched/wait.c lib/find_bit.c fs/file.c lib/aio/aio.c
LIBAIO_OBJS	= $(LIBAIO_SRCS:%.c=$(BUILD_DIR)/lib/%.o)
LIBAIO 	= $(BUILD_DIR)/lib/libaio.so

LIBNFTNL_SRCS	= printk/printk.c lib/netfilter/netfilter.c
LIBNFTNL_OBJS	= $(LIBNFTNL_SRCS:%.c=$(BUILD_DIR)/lib/%.o)
LIBNFTNL	= $(BUILD_DIR)/lib/libnftnl.so

SUBDIRS 	:= entry/ kernel/ ipc/ net/ loader/ printk/

ifeq ($(CONFIG_BLUEFIELD2),y)
SUBDIRS		+= drivers/doca
endif

EXE_SRCS	:= $(shell find $(SUBDIRS) -name '*.c' -print) 
EXE_SRCS 	+= fs/file.c fs/fork.c fs/fs.c fs/read_write.c lib/find_bit.c lib/libc.c lib/regex.c main.c
EXE_OBJS 	= $(EXE_SRCS:%.c=$(BUILD_DIR)/%.o)
EXE_NAME 	= $(BUILD_DIR)/pistachio

all: $(EXE_NAME) $(LIBAIO) $(LIBNFTNL)

C_STANDARD	:= -std=gnu11

MKDIR_P := mkdir -p

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(BUILD_DIR)/lib/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(EXE_NAME): $(EXE_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS)
	@echo 'Finished building: $@'
	@echo ' '

$(LIBAIO): $(LIBAIO_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

$(LIBNFTNL): $(LIBNFTNL_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

clean:
	@rm -rf $(BUILD_DIR)
