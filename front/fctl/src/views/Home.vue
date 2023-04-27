<template>
    <v-container>
        <v-row>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiAlertBox" class="mr-2"></v-icon>Name
                        </span>
                    </template>
                    <template #text>
                        <span style="font-size: 24px">{{ name }}</span>
                    </template>
                </v-card>
            </v-col>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon
                                :icon="mode === 0 ? mdiSignalVariant : mdiBroadcast"
                                class="mr-2"
                            >
                            </v-icon>
                            MODE
                        </span>
                    </template>
                    <template #text>
                        <span style="font-size: 24px">{{ mode === 0 ? 'STA' : 'AP' }}</span>
                    </template>
                </v-card>
            </v-col>
        </v-row>
        <v-row>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiSineWave" class="mr-2"></v-icon>PWM
                        </span>
                    </template>
                    <template #text> <span style="font-size: 24px">25MHz</span> </template>
                </v-card>
            </v-col>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiSpeedometer" class="mr-2"></v-icon>RPM
                        </span>
                    </template>
                    <template #text>
                        <span style="font-size: 24px">{{ rpm }}</span>
                    </template>
                </v-card>
            </v-col>
        </v-row>
        <v-row>
            <v-col class="pa-2">
                <v-card elevation="2">
                    <template #subtitle>
                        <span class="d-flex align-center">
                            <v-icon :icon="mdiFan" class="mr-2"></v-icon>Speed
                        </span>
                    </template>
                    <template #text>
                        <div class="text-right">
                            <span style="font-size: 24px">{{ speed }}</span
                            ><span>%</span>
                        </div>
                        <v-slider
                            class="speed-slider mt-2"
                            step="1"
                            color="purple"
                            :model-value="speed"
                            @update:model-value="change"
                        ></v-slider>
                    </template>
                </v-card>
            </v-col>
        </v-row>
    </v-container>
</template>

<script setup>
import {
    mdiSineWave,
    mdiSpeedometer,
    mdiFan,
    mdiBroadcast,
    mdiSignalVariant,
    mdiAlertBox
} from '@mdi/js'
import { ref, onMounted, onBeforeUnmount } from 'vue'
import axios from 'axios'
import { useDebounceFn } from '@vueuse/core'
const speed = ref(0)
const mode = ref(0)
const name = ref('fctl')
axios.get('/fan/speed').then((res) => {
    speed.value = res.data.speed
})
axios.get('/mode/get').then((res) => {
    mode.value = res.data.mode
})
axios.get('/name').then((res) => {
    name.value = res.data.name
})
const change = useDebounceFn((e) => {
    speed.value = e
    axios.put('/fan/speed', { speed: e })
}, 100)
const rpm = ref(0)
let i
onMounted(() => {
    i = setInterval(() => {
        axios.get('/rpm/get').then((res) => {
            rpm.value = res.data.rpm
        })
    }, 2000)
})
onBeforeUnmount(() => {
    clearInterval(i)
})
</script>
<style lang="scss" scoped>
.speed-slider {
    :deep(.v-input__details) {
        display: none;
    }
}
</style>
