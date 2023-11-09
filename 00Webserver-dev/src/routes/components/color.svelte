<script>
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();
	import ColorPicker from 'svelte-awesome-color-picker';
  export let command = 'interaction'
  export let face = 'null';
  export let param = 'main_color';
  export let label = 'Choose colour';
  let rgb;

  function sendData() {
      dispatch('update', {command:command, face:face, param:param, value:rgb});
      console.log('Color sent');
  }
</script>

<ColorPicker command={command} face={face} param={param} label={label} bind:rgb on:input={sendData}></ColorPicker>
