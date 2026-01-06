ifdef QNX_HOST

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)
include ${BSP_ROOT}/fileset_copy.mk

export ENABLE_TINYVIZ?=OFF

export QC_INSTALL_ROOT=${INSTALL_ROOT_nto}/aarch64le/bin/qcnode

all : install

depends:
	make -C ${BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/build/qnx/qcx/libqcxclient install
	make -C ${BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/build/qnx/qcx-common/osal install
	make -C ${BSP_ROOT}/AMSS/multimedia/compute/fadas install

install:
	@echo Starting QCNODE build...
	./scripts/build/build-target.sh aarch64-qnx .
	cp -v scripts/build/qnx/secpolfiles/qcnode.txt ${BSP_ROOT}/install/secpolfiles/
	cp -v scripts/build/qnx/buildfiles/qc.qcnode.common.system.test.build ${BSP_ROOT}/target/filesets
	echo "Installing to ${QC_INSTALL_ROOT}"
	mkdir -p ${QC_INSTALL_ROOT}
	cp -frv run-aarch64-qnx/opt/qcnode/bin ${QC_INSTALL_ROOT}/
	cp -frv run-aarch64-qnx/opt/qcnode/lib ${QC_INSTALL_ROOT}/

hinstall:

# All clean from here
clean:
	@echo Cleaning QCNODE build...
	rm -rf bld-aarch64-qnx run-aarch64-qnx qcnode-aarch64-qnx.tar.gz

else

hinstall:
install:
clean:

endif