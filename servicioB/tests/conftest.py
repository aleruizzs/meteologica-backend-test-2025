import sys, os
here = os.path.dirname(__file__)
src = os.path.abspath(os.path.join(here, "..", "src"))
if src not in sys.path:
    sys.path.insert(0, src)
