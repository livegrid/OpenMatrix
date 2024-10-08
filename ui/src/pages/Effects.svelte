
<script>
  // @ts-nocheck
  import ModePageLayout from "@/components/ModePageLayout.svelte";
  import EffectCard from "@/components/EffectCard.svelte";
  import Snake from "@/components/effects/Snake.svelte";
  import SimplexNoise from "@/components/effects/SimplexNoise.svelte";
  import Flocking from "@/components/effects/Flocking.svelte";
  import GameOfLife from "@/components/effects/GameOfLife.svelte";
  import LSystem from "@/components/effects/LSystem.svelte";
  import { onDestroy, onMount } from "svelte";
  import { state, selectMode, selectEffect, updateEffectSettings } from "@/store";
  import { get } from 'svelte/store';

  let last_effect = null;
  let loading_effect_id = null;
  
  let showSettings = false;
  let currentEffect = null;

  function openSettings(effect) {
    currentEffect = effect;
    showSettings = true;
  }

  function closeSettings() {
    showSettings = false;
  }

  const effects = [
    {
      id: 1,
      name: 'Simplex Noise',
      image: SimplexNoise
    },
    {
      id: 2,
      name: 'Snake',
      image: Snake
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
  
  function handleSettingsUpdate(effectId, settings) {
    updateEffectSettings(effectId, settings);
  }
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
          on:settings={() => openSettings(effect)}
          name={effect.name}
          image={effect.image}
          effectType={effect.name}
          loading={effect.id === loading_effect_id}
          selected={effect.id === $state?.effects?.selected}
          color={$state?.effects?.colors?.[effect.id] || '#ffffff'}
          onColorChange={(color) => handleSettingsUpdate(effect.id, { color })}
          onSpeedChange={(speed) => handleSettingsUpdate(effect.id, { speed })}
          onScaleChange={(scale) => handleSettingsUpdate(effect.id, { scale })}
          onParticleCountChange={(particleCount) => handleSettingsUpdate(effect.id, { particleCount })}
          onComplexityChange={(complexity) => handleSettingsUpdate(effect.id, { complexity })}
         />
      {/if}
    {/each}
  </dl>
  
{#if showSettings && currentEffect}
<EffectSettingsPopup
  effectType={currentEffect.name}
  color={$state?.effects?.colors?.[currentEffect.id] || '#ffffff'}
  speed={$state?.effects?.speed?.[currentEffect.id] || 50}
  scale={$state?.effects?.scale?.[currentEffect.id] || 50}
  particleCount={$state?.effects?.particleCount?.[currentEffect.id] || 100}
  complexity={$state?.effects?.complexity?.[currentEffect.id] || 3}
  on:close={closeSettings}
  on:colorChange={(e) => handleSettingsUpdate(currentEffect.id, { color: e.detail })}
  on:speedChange={(e) => handleSettingsUpdate(currentEffect.id, { speed: e.detail })}
  on:scaleChange={(e) => handleSettingsUpdate(currentEffect.id, { scale: e.detail })}
  on:particleCountChange={(e) => handleSettingsUpdate(currentEffect.id, { particleCount: e.detail })}
  on:complexityChange={(e) => handleSettingsUpdate(currentEffect.id, { complexity: e.detail })}
/>
{/if}
</ModePageLayout>