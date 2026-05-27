#pragma once

template <typename Children>
void ComposeLoadout(Children children) {
  <Loadout.Frame key="loadout">
    <Loadout.Toolbar activeTab={state.active_tab} />
    <Loadout.Content>{children()}</Loadout.Content>
  </Loadout.Frame>
}
