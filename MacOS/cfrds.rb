class Cfrds < Formula
  desc "ColdFusion RDS protocol library and CLI"
  homepage "https://github.com/bokic/cfrds"
  head "https://github.com/bokic/cfrds.git"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "json-c"
  depends_on "libxml2"

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    system "#{bin}/cfrds", "--version"
  end
end
