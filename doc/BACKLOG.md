# sunlight backlog

Work items for **sunlight** that are known but not yet planned: each one waits here until the owner
schedules it. When an item is picked up, it is planned with the owner first, then tracked in
[`MASTER_PLAN.md`](MASTER_PLAN.md) (or the plan of record at that time) until it is done; its entry
here then says where it went.

Each item says where it came from, what is known so far, and the ideas raised, without deciding
between them.

---

## 1. Linux release archives need a recent GLIBC

**Status: BACKLOG** (reported 2026-11-18 by Scarab, found by Caravellius). Also listed as I26 in
`MASTER_PLAN.md`'s integration follow-ups.

### What was found

Scarab v0.2.0's `linux-x64` archive does not start on Debian 13 (GLIBC 2.41):

```
version 'GLIBC_2.43' not found (required by libsunlight.so / libraylib.so.550)
```

A Linux binary needs at least the GLIBC of the machine it was built on. Scarab builds on
`ubuntu-latest`, and so does sunlight's own `.github/workflows/release.yml` (the `linux-x64`
package job). So sunlight's published Linux archive (v0.39.0 and later) very likely has the same
limit for anyone who uses it directly. Not yet checked on sunlight's own archive: reading the
`GLIBC_*` symbol versions its `.so` files require would confirm it.

Scarab tracks its own side as item 3 of its `SCARAB_TODO.md`. Its fix does not depend on sunlight,
since Scarab builds sunlight from source.

### Ideas raised (not decided)

- Build the Linux archive on the oldest system to support: an older runner such as `ubuntu-22.04`,
  or a container such as `debian:bullseye`.
- Link libstdc++ and libgcc statically.
- Add a release smoke test that loads the library on an old distribution.

### To decide when planned

- Which distributions and GLIBC version the Linux archive should support.
- Whether raylib, tmx and libxml2 (shipped as shared libraries in the archive) need the same
  treatment as sunlight itself.
