package com.ford.iot_robot

import androidx.lifecycle.MutableLiveData

class Servo(val resetPosition:Int = 90) {
    var angle = MutableLiveData(90)
    var speed = MutableLiveData(0)

}

class FirebaseData(val lx:Int=0, val ly: Int=0, val rx: Int=0, val ry: Int=0, val tr: Int=0, val rt:Int=0) {


}