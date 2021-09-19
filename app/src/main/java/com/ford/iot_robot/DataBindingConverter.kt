package com.ford.iot_robot

import android.text.TextUtils
import androidx.databinding.InverseMethod

class DataBindingConverter {
    companion object{
        @InverseMethod("convertIntegerToString")
        @JvmStatic
        fun convertStringToInteger(value: String): Int?{
            if(TextUtils.isEmpty(value) || !TextUtils.isDigitsOnly(value)){
                return null
            }
            return value.toIntOrNull()
        }
        @JvmStatic
        fun convertIntegerToString(value: Int): String{
            return value.toString()
        }
    }
}