ISO=OPL_Accurate_Read_v1.0.iso

all:
	$(MAKE) -C ee all

clean:
	$(MAKE) -C ee clean
	rm -f *.iso
	rm -f iso/*.BIN
	rm -f iso/TEST_OPL.00

# CD = 650MB = 620MiB
# 10x 10MiB + 9x 57MiB = 613
cd_files:
	dd if=/dev/random of=iso/FILE0A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE0B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE1A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE1B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE2A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE2B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE3A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE3B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE4A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE4B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE5A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE5B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE6A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE6B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE7A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE7B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE8A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE8B.BIN bs=1M count=57
	dd if=/dev/random of=iso/FILE9A.BIN bs=1M count=10

iso: all cd_files
	cp ee/opltester.elf iso/TEST_OPL.00
	mkisofs -o $(ISO) iso

sim: iso
	flatpak --filesystem=host run net.pcsx2.PCSX2 $(PWD)/$(ISO)
