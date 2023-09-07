from distutils.core import setup, Extension

def main():
    setup(name="cfrds",
          version="0.1.0",
          description="Python interface for the fputs C library function",
          author="Boris Barbulovski",
          author_email="bbarbulovski@gmail.com",
          ext_modules=[Extension("cfrds", [
              "cfrds.py.c", 
              "../src/cfrds.c", 
              "../src/cfrds_buffer.c", 
              "../src/cfrds_http.c"
            ])]
    )

if __name__ == "__main__":
    main()
