# Sell Cars: behavior and implementation

Describes the exported implementation **0.1.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Alt+Delete twice within five seconds sells the same stopped, empty, locally owned mounted car. The host validates exact native storage identity, ownership and speed below 0.5. Native Exit must succeed. Sale removes the owned entry before crediting half the vanilla garage offer price through CreditShare; failed credit restores ownership. Upgrades have no separate refund. Success removes live/empty storage, prepares removal, notifies crew state and requests save.

## State and lifecycle

Confirmation identity/timer are temporary. Vehicle removal and shared-economy credit use native state/saves rather than a mod ledger. Focus/UI/world/host gates precede mutation; changing the target prevents a stale confirmation. Native bindings are checked, including the retained Decimal division operator.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/sell-cars/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/sell-cars/mod.c)
- [base.h](../source/mods/sell-cars/base.h)

## Verification boundary

This is not a bulk sale or guaranteed crash-atomic transaction. Actual credit sharing, rollback failures, save reload and co-op sale replay require game qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
