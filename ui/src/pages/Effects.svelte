
<script>
  // @ts-nocheck
  import ModePageLayout from "@/components/ModePageLayout.svelte";
  import EffectCard from "@/components/EffectCard.svelte";
  import CellularNoise from "@/components/effects/CellularNoise.svelte";
  import SimplexNoise from "@/components/effects/SimplexNoise.svelte";
  import Flocking from "@/components/effects/Flocking.svelte";
  import GameOfLife from "@/components/effects/GameOfLife.svelte";
  import LSystem from "@/components/effects/LSystem.svelte";
  import { onDestroy, onMount } from "svelte";
  import { state, selectMode, selectEffect } from "@/store";
  import { get } from 'svelte/store';

  let last_effect = null;
  let loading_effect_id = null;

  const effects = [
    {
      id: 1,
      name: 'Simplex Noise',
      image: SimplexNoise
    },
    {
      id: 2,
      name: 'Cellular Noise',
      image: CellularNoise
    },
    {
      id: 3,
      name: 'Flocking',
      image: Flocking
    },
    {
      id: 4,
      name: 'Game of Life',
      image: GameOfLife
    },
    {
      id: 5,
      name: 'L-System',
      image: LSystem
    }
  ];

  const unsubscribe = state.subscribe((value) => {
    if (last_effect !== value?.effects?.selected) {
      last_effect = value?.effects?.selected;
      loading_effect_id = null;
    }
  });

  onMount(() => {
    last_effect = get(state)?.effects?.selected;
  });

  onDestroy(() => {
    unsubscribe();
  });
</script>

<ModePageLayout name={'Effects'} id={1}>
  <!-- <h3 class="text-base font-semibold leading-6 text-gray-900 dark:text-zinc-300">Last 7 days</h3> -->
  <dl class="grid grid-cols-1 gap-5 sm:grid-cols-2 md:grid-cols-2 lg:grid-cols-2 xl:grid-cols-3">
    {#each effects as effect}
      {#if Object.keys($state).length === 0}
        <div role="status" class="animate-pulse">
          <div class="bg-gray-300/50 rounded dark:bg-zinc-700/50 w-full h-[120px]"></div>
        </div>
      {:else}
        <EffectCard
          on:click={() => {
            loading_effect_id = effect.id;
            selectEffect(effect.id);
          }}
          name={effect.name}
          image={effect.image}
          loading={effect.id === loading_effect_id}
          selected={effect.id === $state?.effects?.selected}
          color={$state?.effects?.colors?.[effect.id] || '#ffffff'}
          onColorChange={(color) => updateEffectColor(effect.id, color)}
        />
      {/if}
    {/each}
  </dl>
</ModePageLayout>