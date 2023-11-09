<script>
	import '../app.postcss';
	import './styles.css';
	// import Toggle from './components/toggle.svelte';
	// import Slider from './components/slider.svelte';
	// import Button from './components/button.svelte';

	import { themeChange } from 'theme-change';
	import { onMount } from 'svelte';
	import Toggle from './components/toggle.svelte';
	import Slider from './components/slider.svelte';
	import Color from './components/color.svelte';
	import Facecard from './components/facecard.svelte';
	import Textarea from './components/textarea.svelte';
	import Button from './components/button.svelte';
	import TopNav from './components/top_nav.svelte';
	import WifiForm from './components/wifi_form.svelte';
  import { deviceIP } from './stores';

	let sendMessage;
	let initialized = false;
	let connected = false;
	let pingTimeout = null;
	let mounted = false;
	let currentTab = 'Faces';
	let tempIP = '';
	// let deviceIP = '192.168.1.199';
	let setupIP;
	// let deviceIP = document.location.host;

	let settingsModalState = false;
	let tm;

	onMount(() => {
		themeChange(false);
		mounted = true;

		// let host = 'ws://' + document.location.host + '/ws';
		console.log('mounting ' + $deviceIP);

		let host = `ws://${$deviceIP}/ws`;

		const socket = new WebSocket(host);

		socket.onclose = () => {
			connected = false;
			console.log('Socket closed');
		};

		socket.onopen = () => {
			setTimeout(() => (initialized = true), 2000);
			connected = true;
			console.log('Socket opened');
		};

		socket.onmessage = async (msg) => {
			console.log(msg);
			try {
				// if (msg == '__ping__') {
				// 	socket.send('__pong__');
				// 	connected = true;
				// 	console.log('Ping received');
				// 	return;
				// }
				const json = JSON.parse(msg.data);
				console.log(json);
				if (json.command === 'pong') {
					connected = true;
					// if (pingTimeout) {
					// 	clearTimeout(pingTimeout);
					// }
					// pingTimeout = setTimeout(() => {
					// 	connected = false;
					// }, 6000);
				}
			} catch (err) {
				console.warn(err);
			}
		};

		socket.onerror = (err) => {
			console.warn(err);
			connected = false;
			console.log('Socket error');
		};


		// setInterval(() => {
		// 	if (socket.readyState === 3) {
		// 		connected = false;
		// 		location.reload();
		// 	} else if (socket.readyState === 1) {
		// 		socket.send('{"command":"ping"}');
		// 	} else {
		// 		connected = false;
		// 	}
		// }, 5000);

		sendMessage = (event) => {
			try {
				socket.send(JSON.stringify(event.detail));
				console.log('Sent');
				console.log(JSON.stringify(event.detail));
			} catch (e) {
				console.log('Error in sending');
				console.log(e);
			}
			return;
		};
	});

  function setNewIP() {
    // $deviceIP=setupIP;
      // $deviceIP = setupIP;
      location.reload();
    };
	let tabs = [
		{
			title: 'Faces',
			svg: 'M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6'
		},
		{
			title: 'Text',
			svg: 'M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6'
		},
		{
			title: 'Settings',
			svg: 'M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6'
		}
	];

	let min_max_limits = {
		brightness_min: -127,
		brightness_max: 127,
		contrast_min: -255,
		contrast_max: 255,
		scale_min: 0,
		scale_max: 100,
		speed_min: 0,
		speed_max: 100
	};

	let backgrounds = [
		{
			title: 'Simplex Noise',
			id: 'simplex',
			img: 'simplexNoise.jpg',
			modal_id: 'simplex' + 'modal',
			brightness_slider: 'simplex' + 'brightness',
			contrast_slider: 'simplex' + 'contrast',
			scale_slider: 'simplex' + 'scale',
			speed_slider: 'simplex' + 'speed',
			color_picker: 'simplex' + 'color',
			rgb_val: 'simplex' + 'rgb'
		},
		{
			title: 'Cellular Noise',
			id: 'cellular',
			img: 'cellularNoise.jpg',
			modal_id: 'cellular' + 'modal',
			brightness_slider: 'cellular' + 'brightness',
			contrast_slider: 'cellular' + 'contrast',
			scale_slider: 'cellular' + 'scale',
			speed_slider: 'cellular' + 'speed',
			color_picker: 'cellular' + 'color',
			rgb_val: 'cellular' + 'rgb'
		},
		{
			title: 'SimplexRidged Noise',
			id: 'simplexridged',
			img: 'simplexRNoise.jpg',
			modal_id: 'simplexridged' + 'modal',
			brightness_slider: 'simplexridged' + 'brightness',
			contrast_slider: 'simplexridged' + 'contrast',
			scale_slider: 'simplexridged' + 'scale',
			speed_slider: 'simplexridged' + 'speed',
			color_picker: 'simplexridged' + 'color',
			rgb_val: 'simplexridged' + 'rgb'
		},
		{
			title: 'Text',
			id: 'text',
			img: 'text.jpg',
			modal_id: 'text' + 'modal',
			color_picker: 'text' + 'color',
			text_rgb_val: 'text' + 'textrgb',
			back_rgb_val: 'text' + 'backrgb'
		},
		{
			title: 'Dashboard',
			id: 'dashboard',
			img: 'dashboard.jpg',
			modal_id: 'dashboard' + 'modal',
			color_picker: 'dashboard' + 'color',
			rgb_val: 'dashboard' + 'rgb'
		},
		{
			title: 'Basic',
			id: 'basic',
			img: 'dashboard.jpg',
			modal_id: 'basic' + 'modal',
			color_picker: 'basic' + 'color',
			rgb_val: 'basic' + 'rgb'
		},
		{
			title: 'Stream',
			id: 'stream',
			img: 'dashboard.jpg',
			modal_id: 'stream' + 'modal',
			color_picker: 'stream' + 'color',
			rgb_val: 'stream' + 'rgb'
		}
	];
</script>

<svelte:head>
	<title>Faces</title>
	<meta name="Faces" content="Backgrounds" />
</svelte:head>
<body>
	<!-- {#if !initialized}
		<div
			class="absolute top-0 left-0 right-0 bottom-0 w-full h-full flex flex-col items-center justify-center gap-5 z-20"
		>
			Loading
			<progress class="progress w-56" />
		</div>
	{/if} -->
	<div class="modal" class:modal-open={!initialized}>
		<div class="modal-box">
			<h3 class="font-bold text-lg">Connecting ...</h3>
			<p class="py-4">Enter IP address of the panel</p>
			<input
				type="text"
				placeholder={$deviceIP}
				class="input w-full max-w-xs" bind:value={$deviceIP}
			/>
			<button class="btn" on:click={setNewIP}>Connect</button>
			<!-- <div class="modal-action">
        <label for="loading-modal" class="btn">Yay!</label>
      </div> -->
		</div>
	</div>
	<!-- <div
			class="absolute top-0 left-0 right-0 bottom-0 w-full h-full flex flex-col items-center justify-center gap-5 z-20"
		>
			Attempting to connect ...

			<progress class="progress w-56" />

			<form w-full max-w-xs on:submit|preventDefault={(e) => (deviceIP = tempIP)}>
				<div class="flex flex-row">
					<input
						type="text"
						placeholder={deviceIP}
						class="input input-bordered w-full max-w-xs grid flex-grow"
						bind:value={tempIP}
					/>
					<div class="divider divider-horizontal" />
					<div class=" grid shrink">
						<button class="btn" type="submit">Submit</button>
					</div>
				</div>
			</form>
		</div>
	{:else} -->
	<TopNav />
	<!-- Initial drawer div -->
	<div class="drawer">
		<input id="my-drawer" type="checkbox" class="drawer-toggle" />
		<div class="drawer-content">
			<div class="lg:p-10 sm:p-1">
				<!-- Faces page -->
				{#if currentTab == 'Faces'}
					<div class="lg:columns-3 sm:columns-1 md:columns-2 space-y-6">
						<!-- All faces go here -->
						{#each backgrounds as back}
							<Facecard
								img={back.img}
								title={back.title}
								id={back.id}
								modal_id={back.modal_id}
								on:update={sendMessage}
							/>
						{/each}
					</div>
					<!-- Modals for faces -->
					{#each backgrounds as back}
						<input type="checkbox" id={back.modal_id} class="modal-toggle" />
						<div for={back.modal_id} class="modal">
							<div class="modal-box flex flex-col gap-y-5">
								<h3 class="font-bold text-lg">{back.title}+ settings</h3>

								{#if back.title === 'Text'}
									<div class="flex flex-row gap-5">
										<Color face={back.id} param="text_color" on:update={sendMessage} />
										<Color
											face={back.id}
											param="background_color"
											label="Choose background color"
											on:update={sendMessage}
										/>
									</div>
									<div>
										<Textarea on:update={sendMessage} />
									</div>
								{:else if back.title === 'Dashboard'}
									<div class="flex flex-row gap-5">
										<Color face={back.id} param="main_color" on:update={sendMessage} />
									</div>
								{:else}<div class="flex flex-row gap-5">
										<Color face={back.id} param="main_color" on:update={sendMessage} />
									</div>
									<Slider
										face={back.id}
										param="brightness"
										minValue={min_max_limits.brightness_min}
										maxValue={min_max_limits.brightness_max}
										on:update={sendMessage}
									/>
									<Slider
										face={back.id}
										param="contrast"
										minValue={min_max_limits.contrast_min}
										maxValue={min_max_limits.contrast_max}
										on:update={sendMessage}
									/>
									<Slider
										face={back.id}
										param="scale"
										minValue={min_max_limits.scale_min}
										maxValue={min_max_limits.scale_max}
										on:update={sendMessage}
									/>
									<Slider
										face={back.id}
										param="speed"
										minValue={min_max_limits.speed_min}
										maxValue={min_max_limits.speed_max}
										on:update={sendMessage}
									/>
								{/if}
								<div class="modal-action">
									<label for={back.modal_id} class="btn">Done</label>
								</div>
							</div>
						</div>
					{/each}
				{/if}

				<!-- Settings Window -->
				{#if currentTab == 'Settings'}
					<div class="overflow-x-auto">
						<table class="table w-full">
							<!-- head -->
							<thead>
								<tr>
									<th>Setting</th>
									<th>Status</th>
								</tr>
							</thead>
							<tbody>
								<!-- row 1 -->
								<tr class="hover" on:click={() => (settingsModalState = true)}>
									<td>WiFi</td>
									<td>Disconnected</td>
								</tr>
								<!-- row 2 -->
								<tr>
									<td>Firmware version</td>
									<td>0.9.0</td>
								</tr>
								<!-- row 3 -->
								<tr class="hover" on:click={(e) => console.log('new firmware')}>
									<td>Upload new Firmware</td>
									<td>Upload new Firmware</td>
								</tr>
							</tbody>
						</table>
					</div>
				{/if}
				<div for="wifi-modal" class="modal cursor-pointer" class:modal-open={settingsModalState}>
					<div class="modal-box">
						<WifiForm on:update={sendMessage} />
						<div class="modal-action">
							<button class="btn" on:click={() => (settingsModalState = false)}>Close</button>
						</div>
					</div>
				</div>
			</div>
		</div>

		<!-- Drawer side component -->
		<div class="drawer-side">
			<label for="my-drawer" class="drawer-overlay" />
			<ul class="menu p-4 w-80 bg-base-100 text-base-content">
				<!-- Sidebar content here -->
				{#each tabs as tab}
					<button class="btn btn-ghost" on:click={(e) => (currentTab = tab.title)}>
						<svg
							xmlns="http://www.w3.org/2000/svg"
							class="h-5 w-5"
							fill="none"
							viewBox="0 0 24 24"
							stroke="currentColor"
							><path
								stroke-linecap="round"
								stroke-linejoin="round"
								stroke-width="2"
								d={tab.svg}
							/></svg
						>
						{tab.title}
					</button>
				{/each}
				<li>
					<div>
						<Toggle command={'main'} param={'power'} on:update={sendMessage} /> Power
					</div>
				</li>
				<li>
					<div>
						<Button
							command={'main'}
							param={'rotate'}
							label={'Rotate display'}
							on:update={sendMessage}
						/>
					</div>
				</li>
				<li>
					<div>
						<Slider command={'main'} param={'globalBrightness'} on:update={sendMessage} />
					</div>
				</li>
			</ul>
		</div>
	</div>
	<!-- {:else}
			<input type="checkbox" id="connect" class="modal-toggle modal-open" />
			<div class="modal">
				<div class="modal-box">
					<h3 class="font-bold text-lg">Connect to the panel</h3>

					<form w-full max-w-xs>
						<input
							type="text"
							placeholder="SSID"
							bind:value={deviceIP}
							class="input input-bordered w-full max-w-xs"
						/>
						<input
							type="text"
							placeholder="password"
							bind:value={password}
							class="input input-bordered w-full max-w-xs"
						/>
						<button class="btn" type="submit">Submit</button>
						<input type="text" placeholder="192.168.101.101" class="input w-full max-w-xs" />
						<div class="modal-action">
							<label for="my-modal" class="btn">Yay!</label>
						</div>
					</form>
				</div>
			</div> -->
</body>

<style>
</style>
