<script>
	import { createEventDispatcher } from 'svelte';
	const dispatch = createEventDispatcher();
	let value = false;
	export let command = 'interaction';
  export let param = 'null';
	function sendData() {
		dispatch('update', { command:command, param:param, value:value });
		console.log('Toggle sent');
	}
</script>

<input type="checkbox" command={command} param={param} class="toggle toggle-sm" bind:checked={value} on:click={sendData} />