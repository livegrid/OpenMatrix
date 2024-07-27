
<script>
  import EffectCard from "@/components/EffectCard.svelte";
  import CellularNoise from "@/components/effects/CellularNoise.svelte";
  import SimplexNoise from "@/components/effects/SimplexNoise.svelte";
  import Flocking from "@/components/effects/Flocking.svelte";
  import GameOfLife from "@/components/effects/GameOfLife.svelte";
  import LSystem from "@/components/effects/LSystem.svelte";

  import { state } from "@/store";

  const effects = [
    {
      id: 1,
      subtitle: 'Simplex Noise',
      image: SimplexNoise
    },
    {
      id: 2,
      subtitle: 'Cellular Noise',
      image: CellularNoise
    },
    {
      id: 3,
      subtitle: 'Flocking',
      image: Flocking
    },
    {
      id: 4,
      subtitle: 'Game of Life',
      image: GameOfLife
    },
    {
      id: 5,
      subtitle: 'L-System',
      image: LSystem
    }
  ];
</script>
<!-- Your content -->
<div>
  <div class="flex flex-wrap items-center gap-6 sm:flex-nowrap sm:justify-between">
    <h1 class="text-base text-lg font-semibold leading-7 text-gray-900 dark:text-zinc-300">Effects</h1>
    {#if $state.mode === 1}
      <button disabled class="flex flex-row items-center uppercase gap-x-3 text-emerald-600 dark:text-emerald-700">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5"><circle cx="12" cy="12" r="10"/><path d="m9 12 2 2 4-4"/></svg>
        Playing
      </button>
    {:else}
      <button class="flex flex-row items-center uppercase gap-x-3 text-gray-400 hover:text-gray-500 transition-colors">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5"><circle cx="12" cy="12" r="10"/><polygon points="10 8 16 12 10 16 10 8"/></svg>
        Play
      </button>
    {/if}
  </div>
</div>

<div class="mt-8">
  <!-- <h3 class="text-base font-semibold leading-6 text-gray-900 dark:text-zinc-300">Last 7 days</h3> -->
  <dl class="grid grid-cols-1 gap-5 sm:grid-cols-2 md:grid-cols-2 lg:grid-cols-2 xl:grid-cols-3">
    {#each effects as effect}
      <EffectCard image={effect.image} subtitle={effect.subtitle} selected={effect.id === $state.effects.selected}/>
    {/each}
  </dl>
</div>