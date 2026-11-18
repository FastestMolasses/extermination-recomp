"""Original page-program replay for later-level texture discovery.

The page parser is chain_page_model's (it has replayed the 001E9280 surface
packet D_00234FE0 itself since that program joined its table, together with
every later page program); this module keeps the exporters' entry point
`captured` and the surface constants. Its own copy of the parser, which
knew only the programs of its day, was retired in test-health lane 2
(2026-11-18): chain_page_model refuses an unknown MPG source the same way.
"""
import chain_page_model as M

PROGRAM_SURFACE = M.PROGRAM_SURFACE
SURFACE_MPG = M.SURFACE_MPG
BATCHED = M.BATCHED
Page = M.Page


def captured(capture):
    """All textured page primitives; owner units have separate model readers."""
    read = M.ram_reader(capture.ram)
    start = M.latest_start(capture.ram)
    probe = M.Page(read)
    probe.dma(start)
    skip = {a for _, tid, _, a in probe.transfers
            if tid == 5 and M.ARENA <= a < 0x800000}
    page = Page(read, skip_calls=skip)
    page.run(start)
    return page, skip
