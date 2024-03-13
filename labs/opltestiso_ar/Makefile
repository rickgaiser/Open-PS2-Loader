ISO=OPL_Accurate_Read_v1.1.iso

all:
	$(MAKE) -C ee all

clean:
	$(MAKE) -C ee clean
	rm -f *.iso
	rm -f iso/*.BIN
	rm -f iso/TEST_OPL.00

# CD = 333000 2KiB sectors = 650MiB
# 10x 10MiB + 9x 61MiB = 649MiB
cd_files:
	dd if=/dev/random of=iso/FILE0A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE0B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE1A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE1B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE2A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE2B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE3A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE3B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE4A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE4B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE5A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE5B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE6A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE6B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE7A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE7B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE8A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE8B.BIN bs=1M count=61
	dd if=/dev/random of=iso/FILE9A.BIN bs=1M count=10

# DVD = 2298496 2KiB sectors = 4489MiB
# 10x 10MiB + 9x 487MiB = 4483MiB
dvd_files:
	dd if=/dev/random of=iso/FILE0A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE0B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE1A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE1B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE2A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE2B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE3A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE3B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE4A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE4B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE5A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE5B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE6A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE6B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE7A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE7B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE8A.BIN bs=1M count=10
	dd if=/dev/random of=iso/FILE8B.BIN bs=1M count=487
	dd if=/dev/random of=iso/FILE9A.BIN bs=1M count=10

iso: all
	cp ee/opltester.elf iso/TEST_OPL.00
	mkisofs -udf -v -r -T -J -o $(ISO) iso

sim:
	flatpak --filesystem=host run net.pcsx2.PCSX2 $(PWD)/$(ISO)
