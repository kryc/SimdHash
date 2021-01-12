
.PHONY: all
all: simdhash simdhashtest

.PHONY: simdhash
simdhash:
	make -C SimdHash/

.PHONY: simdhashtest
simdhashtest: simdhash
	make -C SimdHashTest/

.PHONY: clean
clean:
	make -C SimdHashTest/ clean
	make -C SimdHash/ clean