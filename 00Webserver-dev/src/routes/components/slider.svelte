<script>
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();
  let value = 50;
  export let command = 'interaction';
  export let face = 'null';
  export let param = 'null';
  export let minValue = 0;
  export let maxValue = 100;
  export let classSlider = "range range-primary";
  function sendData() {
      dispatch('update', { command:command, face: face, param: param, value: value });
      console.log('Slider sent');
  }
</script>
<div>
  {param}
  <input type="range" command={command} class={classSlider} face={face} param={param} min={minValue} max={maxValue} bind:value={value} on:change={sendData} />
</div>
