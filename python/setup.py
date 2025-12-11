from distutils.core import setup, Extension

def main():
    setup(name="cfrds",
          description="Python interface for ColdFusion RDS service.",
          author="Boris Barbulovski",
          author_email="bbarbulovski@gmail.com",
          ext_modules=[Extension("cfrds", [
              "cfrds.py.c",
              "../src/cfrds.c",
              "../src/cfrds_buffer.c",
              "../src/cfrds_http.c"
            ],
            include_dirs=['../include', '/usr/include/libxml2'],
            libraries=['xml2'])]
        version="0.9.8",
    )

if __name__ == "__main__":
    main()
