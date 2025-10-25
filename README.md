# deadlock-detector

## I/O Format

### Matrix mode
Input:
- Dòng 1: `N M`
- `N` dòng Allocation (M số mỗi dòng)
- `N` dòng Request/Need (M số mỗi dòng)
- Dòng cuối: `M` số Available

Output:
- `NO DEADLOCK` **hoặc**
- `DEADLOCK on k process(es): P<i> P<j> ...`

### Wait-For Graph (WFG) mode
Input:
- Dòng 1: `P E`
- `E` dòng `u v` nghĩa là `Pu` đang chờ `Pv` (0-index)

Output:
- `NO DEADLOCK` **hoặc**
- `DEADLOCK cycle: P<i0> P<i1> ... P<ik> P<i0>`

## Build
make
