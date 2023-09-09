from distutils.core import setup, Extension

def main():
    setup(name="cfrds",
          version="0.6.0",
          description="Python interface for ColdFusion RDS service.",
          author="Boris Barbulovski",
          author_email="bbarbulovski@gmail.com",
          ext_modules=[Extension("cfrds", [
              "cfrds.py.c", 
              "../src/cfrds.c", 
              "../src/cfrds_buffer.c",
              "../src/cfrds_http.c"
            ],  include_dirs=['../include'])]
    )

if __name__ == "__main__":
    main()
