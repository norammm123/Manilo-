// utils/ble.js - 蓝牙手套通信封装 (适配 HC-08 模块)
// HC-08 使用标准的蓝牙串口透传 UUID
const GLOVE_SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb'
const GLOVE_CHAR_UUID = '0000ffe1-0000-1000-8000-00805f9b34fb'

// HC-08 默认设备名，如果你的模块改过名，请修改这里
const DEVICE_NAME = 'HC-08'

class GloveBLE {
  constructor() {
    this.deviceId = null
    this.serviceId = GLOVE_SERVICE_UUID
    this.charId = GLOVE_CHAR_UUID
    this.connected = false
    this._listeners = []
  }

  // 初始化蓝牙适配器
  init() {
    const self = this
    return new Promise((resolve, reject) => {
      // 先检查是否已初始化
      wx.getBluetoothAdapterState({
        success: (res) => {
          console.log('[BLE] 蓝牙状态:', res)
          // 蓝牙未开启
          if (!res.powered) {
            resolve({ success: false, error: '请先开启手机蓝牙' })
            return
          }
          // 已开启，直接成功
          resolve({ success: true })
        },
        fail: () => {
          // 未初始化，执行打开
          wx.openBluetoothAdapter({
            success: () => {
              console.log('[BLE] 初始化成功')
              resolve({ success: true })
            },
            fail: (err) => {
              // already opened 也算成功
              if (err.errMsg && err.errMsg.includes('already opened')) {
                console.log('[BLE] 蓝牙已开启')
                resolve({ success: true })
                return
              }
              console.error('[BLE] 初始化失败:', err)
              if (err.errCode === 10001) {
                resolve({ success: false, error: '请打开手机蓝牙' })
              } else {
                resolve({ success: false, error: err.errMsg })
              }
            }
          })
        }
      })
    })
  }

  // 扫描并连接设备
  scanAndConnect(timeout = 10000) {
    return new Promise((resolve, reject) => {
      let found = false
      let timer = null
      
      // 先停止可能正在进行的扫描
      wx.stopBluetoothDevicesDiscovery({
        complete: () => {
          // 监听发现设备
          wx.onBluetoothDeviceFound((res) => {
            const devices = res.devices || []
            const device = devices.find(d => 
              d.name === DEVICE_NAME || 
              d.localName === DEVICE_NAME
            )
            if (device && !found) {
              found = true
              if (timer) clearTimeout(timer)
              wx.stopBluetoothDevicesDiscovery()
              this.connect(device.deviceId)
                .then(() => resolve(device))
                .catch(reject)
            }
          })

          // 开始扫描
          wx.startBluetoothDevicesDiscovery({
            services: [this.serviceId],
            allowDuplicatesKey: false,
            success: () => {
              console.log('[BLE] 开始扫描设备:', DEVICE_NAME)
            },
            fail: (err) => reject(err)
          })

          // 超时处理
          timer = setTimeout(() => {
            if (!found) {
              wx.stopBluetoothDevicesDiscovery()
              reject(new Error('扫描超时，未找到 ' + DEVICE_NAME))
            }
          }, timeout)
        }
      })
    })
  }

  // 连接指定设备
  connect(deviceId) {
    return new Promise((resolve, reject) => {
      this.deviceId = deviceId
      
      wx.createBLEConnection({
        deviceId,
        timeout: 10000,
        success: () => {
          console.log('[BLE] 连接成功')
          
          // 获取服务
          wx.getBLEDeviceServices({
            deviceId,
            success: (res) => {
              console.log('[BLE] 服务列表:', res.services)
              
              // 获取特征值
              wx.getBLEDeviceCharacteristics({
                deviceId,
                serviceId: this.serviceId,
                success: (charRes) => {
                  console.log('[BLE] 特征值列表:', charRes.characteristics)
                  
                  // 开启通知
                  wx.notifyBLECharacteristicValueChange({
                    deviceId,
                    serviceId: this.serviceId,
                    characteristicId: this.charId,
                    state: true,
                    success: () => {
                      console.log('[BLE] 通知已开启')
                      this.connected = true
                      this._emit('connect', { deviceId })
                      resolve()
                    },
                    fail: reject
                  })
                },
                fail: reject
              })
            },
            fail: reject
          })
          
          // 监听数据
          wx.onBLECharacteristicValueChange((res) => {
            const data = this._parseData(res.value)
            this._emit('data', data)
          })
          
          // 监听断开
          wx.onBLEConnectionStateChange((res) => {
            if (!res.connected) {
              this.connected = false
              this._emit('disconnect', res)
            }
          })
        },
        fail: reject
      })
    })
  }

  // 发送指令到手套
  send(cmd) {
    if (!this.connected || !this.deviceId) {
      console.warn('[BLE] 未连接，无法发送')
      return false
    }
    
    try {
      const json = JSON.stringify(cmd)
      const buffer = this._str2ab(json)
      
      wx.writeBLECharacteristicValue({
        deviceId: this.deviceId,
        serviceId: this.serviceId,
        characteristicId: this.charId,
        value: buffer,
        success: () => console.log('[BLE] 发送成功:', cmd),
        fail: (err) => console.error('[BLE] 发送失败:', err)
      })
      return true
    } catch (err) {
      console.error('[BLE] 发送异常:', err)
      return false
    }
  }

  // 断开连接
  disconnect() {
    if (this.deviceId) {
      wx.closeBLEConnection({
        deviceId: this.deviceId,
        complete: () => {}
      })
    }
    this.deviceId = null
    this.connected = false
  }

  // 事件监听
  on(event, callback) {
    this._listeners.push({ event: event, callback: callback })
  }

  _emit(event, data) {
    this._listeners
      .filter(function(l) { return l.event === event })
      .forEach(function(l) { l.callback(data) })
  }

  // 数据解析
  _parseData(buffer) {
    const bytes = new Uint8Array(buffer)
    let str = ''
    for (let i = 0; i < bytes.length; i++) {
      str += String.fromCharCode(bytes[i])
    }
    try {
      return JSON.parse(str)
    } catch (e) {
      return { raw: str }
    }
  }

  // 字符串转 ArrayBuffer
  _str2ab(str) {
    const buf = new ArrayBuffer(str.length)
    const view = new Uint8Array(buf)
    for (let i = 0; i < str.length; i++) {
      view[i] = str.charCodeAt(i)
    }
    return buf
  }
}

module.exports = new GloveBLE()
