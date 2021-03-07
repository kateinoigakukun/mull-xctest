class MullXctest < Formula
  version "0.1.1"
  desc "Experimental mutation testing tool for Swift and XCTest powered by mull"
  homepage "https://github.com/kateinoigakukun/mull-xctest"
  url "https://github.com/kateinoigakukun/mull-xctest/releases/download/0.1.1/mull-xctest-x86_64-apple-darwin.tar.gz"
  sha256 "2f26da6510a3b43a74ff095b6fc251a1e04b95f6d5f957d714357a1be2925d9f"

  def install
    opt_prefix.install Dir["local/bin"]
  end
end
