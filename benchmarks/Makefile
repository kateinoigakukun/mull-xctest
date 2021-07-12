MULL_TOOLS := /usr/local/opt/mull-xctest/bin/mull-tools

# all: swift-algorithms.benchmark swift-collections.benchmark

# %: build/%.mull build/%.muter
%.benchmark:
	$(MAKE) build/$@.mull
	$(MAKE) build/$@.muter

build/swift-algorithms.checkout:
	git clone https://github.com/apple/swift-algorithms.git build/swift-algorithms
	cd build/swift-algorithms && swift package resolve
	cp muter.conf.json build/swift-algorithms/muter.conf.json
	touch build/swift-algorithms.checkout

build/swift-algorithms.benchmark.mull: build/swift-algorithms.checkout
	cd build/swift-algorithms && \
	swift package clean && \
	$(CURDIR)/scripts/time.sh "$(CURDIR)/build/swift-algorithms.benchmark.mull.time" \
	  swift build --build-tests \
	     -Xswiftc -embed-bitcode \
	     -Xswiftc -tools-directory -Xswiftc "${MULL_TOOLS}" \
	     -Xlinker -Xmull -Xlinker --include-path='.*/swift-algorithms/Sources/.*/.*.swift$$' \
	     -Xlinker -Xmull -Xlinker --workers=1
	${MULL_TOOLS}/mull-dump-mutants \
		build/swift-algorithms/.build/x86_64-apple-macosx/debug/swift-algorithmsPackageTests.xctest/Contents/MacOS/swift-algorithmsPackageTests \
		> build/swift-algorithms.benchmark.mull

build/swift-algorithms.benchmark.muter: build/swift-algorithms.checkout
	cd build/swift-algorithms && \
	swift package clean && \
	$(CURDIR)/scripts/time.sh "$(CURDIR)/build/swift-algorithms.benchmark.muter.time" \
	  $(CURDIR)/scripts/muter.sh --output-json > $(CURDIR)/build/swift-algorithms.benchmark.muter


build/swift-collections.checkout:
	git clone https://github.com/apple/swift-collections.git build/swift-collections
	cd build/swift-collections && swift package resolve
	cp muter.conf.json build/swift-collections/muter.conf.json
	touch build/swift-collections.checkout

build/swift-collections.benchmark.mull: build/swift-collections.checkout
	cd build/swift-collections && \
	swift package clean && \
	$(CURDIR)/scripts/time.sh "$(CURDIR)/build/swift-collections.benchmark.mull.time" \
	  swift build --build-tests \
	     -Xswiftc -embed-bitcode \
	     -Xswiftc -tools-directory -Xswiftc "${MULL_TOOLS}" \
	     -Xlinker -Xmull -Xlinker --include-path='.*/swift-collections/Sources/.*/.*.swift$$' \
	     -Xlinker -Xmull -Xlinker --workers=1
	${MULL_TOOLS}/mull-dump-mutants \
		build/swift-collections/.build/x86_64-apple-macosx/debug/swift-collectionsPackageTests.xctest/Contents/MacOS/swift-collectionsPackageTests \
		> build/swift-collections.benchmark.mull

build/swift-collections.benchmark.muter: build/swift-collections.checkout
	cd build/swift-collections && \
	swift package clean && \
	$(CURDIR)/scripts/time.sh "$(CURDIR)/build/swift-collections.benchmark.muter.time" \
	  $(CURDIR)/scripts/muter.sh --output-json > $(CURDIR)/build/swift-collections.benchmark.muter
