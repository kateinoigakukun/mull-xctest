MULL_TOOLS := /usr/local/opt/mull-xctest/bin/mull-tools
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

%.benchmark: build/%.benchmark.mull build/%.benchmark.muter

all: swift-algorithms.benchmark
